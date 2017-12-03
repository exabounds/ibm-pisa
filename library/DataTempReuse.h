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

class DataTempReuse;

#ifndef LLVM_DATA_TEMP_REUSE__H
#define LLVM_DATA_TEMP_REUSE__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "splay.h"
#include "JSONmanager.h"

#include<pthread.h>

class DataTempReuse: public InstructionAnalysis {

public:
    // Cache line size. 0 means disable this option.
    int cache_line_size;

    // This variable is 0 or 1, for disable or enable emmory footprint analysis, respectively.
    int mem_footprint;

    // How far each bin should be placed. 0 means powers of 2 granularity.
    unsigned long long resolution;

    // The final value of reuse distance to which that resolution is required. Then, powers of 2 granularity.
    unsigned long long resolution_final_bin;

    // Map between memory address and last issue cycle access
    map<unsigned long long, unsigned long long> LastMemoryAccess;

    // Map used to monitor all the references at byte granularity - used by the memory footprint analysis.
    map<unsigned long long, bool> LastMemoryAccessByte;

    // Data structure used to compute the number of different memory accesses 
    // since the last access of the current memory address
    tree *DistanceTree;

    // Data structure for storing reuse distance / distribution
    map<unsigned long long, unsigned long long> DistanceDistributionMap;

    pthread_mutex_t* print_lock;

    DataTempReuse(Module *M, 
                  int data_cache_line_size, 
                  int data_reuse_distance_resolution,
                  int data_reuse_distance_resolution_final_bin,  
                  int thread_id,
                  int processor_id, 
                  int mem_footprint, 
                  pthread_mutex_t* print_lock);

    ~DataTempReuse();

    void visit(Instruction &I, unsigned long long CurrentIssueCycle);
    
    void JSONdump(JSONmanager *JSONwriter, 
                  unsigned long long norm, 
                  unsigned long long sharedBytes, 
                  unsigned long long sharedAccesses);
};


#include "WKLDchar.h"

class WKLDchar;
void compute_shared_memory(vector<WKLDchar>& WKLDcharForThreads, 
                           unsigned long long options,
                           unsigned long long &sharedBytes,
                           unsigned long long &sharedAccesses);

#endif // LLVM_DATA_TEMP_REUSE__H
