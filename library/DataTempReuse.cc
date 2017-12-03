/*******************************************************************************
 * (C) Copyright IBM Corporation 2017
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Algorithms & Machines team
 *******************************************************************************/ 

#include "DataTempReuse.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

DataTempReuse::DataTempReuse(Module *M, 
                             int data_cache_line_size, 
                             int data_reuse_distance_resolution,
                             int data_reuse_distance_resolution_final_bin, 
                             int thread_id,
                             int processor_id, 
                             int mem_footprint, 
                             pthread_mutex_t* print_lock) :
    InstructionAnalysis(M, thread_id, processor_id) {
        
    this->cache_line_size = data_cache_line_size;
    this->resolution = data_reuse_distance_resolution;
    this->resolution_final_bin = data_reuse_distance_resolution_final_bin;
    this->DistanceTree = NULL;
    this->mem_footprint = mem_footprint;
    this->print_lock = print_lock;
}

DataTempReuse::~DataTempReuse() {
    DeleteAll(DistanceTree);
}

void DataTempReuse::JSONdump(JSONmanager *JSONwriter,
                             unsigned long long NormFactor, 
                             unsigned long long sharedBytesAcrossThreads, 
                             unsigned long long sharedAccessesAcrossThreads) {

    // Several space problems when using resolution (otherwise power of 2 is efficient)                             
    bool compressOutput = resolution > 0; 
    
    map<unsigned long long, unsigned long long>::iterator it = DistanceDistributionMap.begin();
    JSONwriter->StartArray();
    JSONwriter->StartObject();
    
        JSONwriter->String("cacheLineSize");
        JSONwriter->Uint64(cache_line_size);
        JSONwriter->String("statistics");
        JSONwriter->StartObject();
            
            if(!compressOutput){ // This is redundant.    
                JSONwriter->String("data");
                JSONwriter->StartArray();
                for (; it != DistanceDistributionMap.end(); it++)  {
                    JSONwriter->StartArray();
                    JSONwriter->Uint64(it->first);
                    JSONwriter->Uint64(it->second);
                    JSONwriter->EndArray();
                }
                JSONwriter->EndArray();
            }
        
            JSONwriter->String("dataCDF");
            JSONwriter->StartArray();
            unsigned long long sum = 0;
            it = DistanceDistributionMap.begin();
            std::string lastWrote("");
            for (; it != DistanceDistributionMap.end(); it++) {
                sum += it->second;        
                std::string newStr = boost::lexical_cast<std::string>(double4((double)sum/NormFactor));
                if(( newStr != lastWrote ) || !compressOutput){
                  lastWrote = newStr;

                  JSONwriter->StartArray();
                  JSONwriter->Uint64(it->first);
                  JSONwriter->Double(double4((double)sum/NormFactor));
                  JSONwriter->EndArray();
                }
            }
            JSONwriter->EndArray();
            
        JSONwriter->EndObject();
        
    JSONwriter->EndObject();
    
    JSONwriter->EndArray();
 
    // There are two cases when we print the memory_footprint information
    // 1. When the DTR analysis is run with cache_line_size disabled.
    // 2. When the DTR analysis is run with cache_line_size and memory footprint enabled.
    if ((!cache_line_size) || (mem_footprint)) {
        JSONwriter->String("memoryFootprint");
        JSONwriter->StartObject();

        JSONwriter->String("shared_addresses_byte_granularity");
        JSONwriter->Uint64(sharedBytesAcrossThreads);

        if (mem_footprint == 1) {
            if (cache_line_size) {
                JSONwriter->String("total_distinct_addresses_byte_granularity");
                JSONwriter->Uint64(LastMemoryAccessByte.size());
            } else {
                JSONwriter->String("total_distinct_addresses_byte_granularity");
                JSONwriter->Uint64(LastMemoryAccess.size());
            }
        }
        else
            if (!cache_line_size) {
                JSONwriter->String("total_distinct_addresses_byte_granularity");
                JSONwriter->Uint64(LastMemoryAccess.size());
            }

        JSONwriter->String("shared_accesses");
        JSONwriter->Uint64(sharedAccessesAcrossThreads);

        JSONwriter->String("total_distinct_accesses_starting_addresses");
        JSONwriter->Uint64(TreeSize(DistanceTree));

        JSONwriter->String("cache_line_size");
        if (!cache_line_size) 
            JSONwriter->Uint64(1);
        else 
            JSONwriter->Uint64(cache_line_size);

        JSONwriter->EndObject();
    }
}

void DataTempReuse::visit(Instruction &I, unsigned long long CurrentIssueCycle) {
    if (I.getOpcode() != Instruction::Load && I.getOpcode() != Instruction::Store)
        return;

    unsigned long long MemoryAddress = 0, MemoryAddressReal = 0;
    unsigned MemoryAccessSize = getMemoryAccessSize(I);

    MemoryAddressReal = (unsigned long long)getMemoryAddress(&I, thread_id);

    if (cache_line_size)
        MemoryAddress = MemoryAddressReal / cache_line_size;
    else
        MemoryAddress = MemoryAddressReal;

    bool found = false;
    unsigned long long distance = 0;
    unsigned long long PreviousIssueCycle = 0;

    // pthread_mutex_lock(print_lock);
    // cout << "thread_id=" << thread_id << " address=" << MemoryAddressReal << " size=" << MemoryAccessSize << "\n";
    // pthread_mutex_unlock(print_lock);

    // We perform this action for each byte accessed
    for (unsigned i = 0; i < MemoryAccessSize; i++) {
        unsigned long long PreviousIssueCycleTmp = 0;
        unsigned long long CurrentMemoryAccess = MemoryAddress + i;

        // Extract the last cycle (timestamp) of the current memory access
        const auto it = LastMemoryAccess.find(CurrentMemoryAccess);
        if (it != LastMemoryAccess.end()) {
            found = true;
            PreviousIssueCycleTmp = it->second;
            it->second = CurrentIssueCycle;
        } else {
            // Update the access with the new timestamp (issue cycle)
            LastMemoryAccess.insert(pair<unsigned long long, unsigned long long>(CurrentMemoryAccess, CurrentIssueCycle));
        }

        // If the memory access was previously accessed
        if (found) {
            unsigned long long distance_tmp = 0;
            
            // Compute the reuse distance using the splay tree.
            // Distance = no. different accesses that were performed
            // since the last access to the current memory access.
            // Note! this is the number of accesses, not the number of the bytes accessed!
            // If we have a single access of 4 bytes this will count only once.
            DistanceTree = ComputeDistance(PreviousIssueCycleTmp, DistanceTree, &distance_tmp);
            if (DistanceTree && DistanceTree->key == PreviousIssueCycleTmp)
                // We always select the minimum reuse distance
                if (!distance || distance_tmp < distance) {
                    distance = distance_tmp;
                    PreviousIssueCycle = PreviousIssueCycleTmp;
                }
        }

        // If we use cache_line_size then we will not do this operation for every
        // single byte of the access since an access will move an entire line into the cache.
        if (cache_line_size) {
            if (mem_footprint == 1) {
                for (unsigned j = 0; j < MemoryAccessSize; j++) {
                    unsigned long long CurrentAddress = MemoryAddressReal + j;
                    const auto it = LastMemoryAccessByte.find(CurrentAddress);
                    if (it == LastMemoryAccessByte.end()) {
                        LastMemoryAccessByte.insert(pair<unsigned long long, bool>(CurrentAddress, 1));
                    }
                }
            }
            break;
        }
    }

    // Update splay with the new issue cycle (timestamp).
    // The update is performed per access, not per byte.
    if (found)
        DistanceTree = Update(PreviousIssueCycle, CurrentIssueCycle, DistanceTree);
    else
        DistanceTree = Insert(CurrentIssueCycle, DistanceTree);
   
    if ((distance > resolution_final_bin) || (resolution < 1))
        distance = upperPowerOfTwo(distance);
    else
        distance = distance + (resolution - (distance % resolution));

    if (found) {
        // If the memory access was previously accessed then update
        // the reuse distribution tree.
        const auto it = DistanceDistributionMap.find(distance);
        if (it == DistanceDistributionMap.end())
            DistanceDistributionMap.insert(pair<unsigned long long, unsigned long long>(distance, 1));
        else
            it->second++;
    }
}

// This function is used for the memory footprint analysis.
// There are two cases when we print memory_footprint information:
// 1. When the DTR analysis is run without the cache_line_size enabled.
// 2. When the DTR analysis is run with the cache_line_size && the memory footprint parameters enabled.
    
void compute_shared_memory(vector<WKLDchar>& WKLDcharForThreads, 
                           unsigned long long options,
                           unsigned long long &sharedBytes,
                           unsigned long long &sharedAccesses) {
                               
    vector<pair<unsigned long long,bool> > intersectOutput[2];
    vector<pair<unsigned long long,unsigned long long> > intersectOutputLong[2];
    
    if (WKLDcharForThreads.size() > 1)
        if (options & ANALYZE_DTR) {
            if (InitializedThreads[0] && InitializedThreads[1]) {
                if (!WKLDcharForThreads[0].dtr->cache_line_size) {
                    intersectOutputLong[0].resize(WKLDcharForThreads[0].dtr->LastMemoryAccess.size());
                    intersectOutputLong[1].resize(WKLDcharForThreads[0].dtr->LastMemoryAccess.size());
                    intersectTwoMapsLong(WKLDcharForThreads[0].dtr->LastMemoryAccess, WKLDcharForThreads[1].dtr->LastMemoryAccess, intersectOutputLong[0]);

                    unsigned long realSize = 2;
                    
                    // This might be buggy if the max-expected-threads parameter is not set. (if not set: all threads are expected in increasing order)
                    for(unsigned long i = 2; i < WKLDcharForThreads.size(); i++)
                          if (InitializedThreads[i]) {
                            intersectMoreLong(intersectOutputLong[i%2], WKLDcharForThreads[i].dtr->LastMemoryAccess, intersectOutputLong[(i+1)%2]);
                            realSize++;
                          }

                    sharedBytes = intersectOutputLong[realSize%2].size();
                    sharedAccesses = intersectOutputLong[realSize%2].size();
                }

                if (WKLDcharForThreads[0].dtr->cache_line_size && (options & ANALYZE_MEM_FOOTPRINT)) {
                    intersectOutput[0].resize(WKLDcharForThreads[0].dtr->LastMemoryAccessByte.size());
                    intersectOutput[1].resize(WKLDcharForThreads[0].dtr->LastMemoryAccessByte.size());
                    intersectTwoMaps(WKLDcharForThreads[0].dtr->LastMemoryAccessByte, WKLDcharForThreads[1].dtr->LastMemoryAccessByte, intersectOutput[0]);

                    unsigned long realSize = 2;
                    
                    // This might be buggy if the max-expected-threads parameter is not set. (if not set: all threads are expected in increasing order)
                    for(unsigned long i = 2; i < WKLDcharForThreads.size(); i++) {
                        if (InitializedThreads[i]) {
                            intersectMore(intersectOutput[i%2], WKLDcharForThreads[i].dtr->LastMemoryAccessByte, intersectOutput[(i+1)%2]);
                            realSize++;
                        }
                    }

                    sharedBytes = intersectOutput[realSize%2].size();

                    intersectOutputLong[0].resize(WKLDcharForThreads[0].dtr->LastMemoryAccess.size());
                    intersectOutputLong[1].resize(WKLDcharForThreads[0].dtr->LastMemoryAccess.size());
                    intersectTwoMapsLong(WKLDcharForThreads[0].dtr->LastMemoryAccess, WKLDcharForThreads[1].dtr->LastMemoryAccess, intersectOutputLong[0]);

                    for(unsigned long i = 2; i < WKLDcharForThreads.size(); i++) {
                            if (InitializedThreads[i]) {
                                intersectMoreLong(intersectOutputLong[i%2], WKLDcharForThreads[i].dtr->LastMemoryAccess, intersectOutputLong[(i+1)%2]);
                            }
                    }

                    sharedAccesses = intersectOutputLong[realSize%2].size();
                }
            }
        }
}
