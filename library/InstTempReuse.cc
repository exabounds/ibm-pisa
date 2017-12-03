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

#include "InstTempReuse.h"

InstTempReuse::InstTempReuse(Module *M, int inst_cache_line_size, int inst_size, int thread_id, int processor_id) :
    InstructionAnalysis(M, thread_id, processor_id) {
        
    this->cache_line_size = inst_cache_line_size;
    this->inst_size = inst_size;
    this->DistanceTree = NULL;

    if (this->cache_line_size && this->inst_size) {
        // Initialize map of instructions
        unsigned long long index = this->cache_line_size;
        for (Module::iterator F = M->begin(), N = M->end(); F != N; ++F)
            for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
                for (BasicBlock::iterator I = BB->begin(), J = BB->end(); I != J; ++I) {
                    InstructionsIndex.insert(pair<Instruction *, unsigned long long>((Instruction*)I, index));
                    index++;
                }
    }
}

InstTempReuse::~InstTempReuse() {
    DeleteAll(DistanceTree);
}

void InstTempReuse::JSONdump(JSONmanager *JSONwriter, unsigned long long norm) {
    JSONwriter->StartObject();
    JSONwriter->String("instructionSize");
    JSONwriter->Uint64(inst_size);
    JSONwriter->String("cacheLineSize");
    JSONwriter->Uint64(cache_line_size);    
    JSONwriter->String("statistics");
    JSONwriter->StartObject();
    
        JSONwriter->String("instructions");
        JSONwriter->StartArray();
        map<unsigned long long, unsigned long long>::iterator it = DistanceDistributionMap.begin();
        for (; it != DistanceDistributionMap.end(); it++)  {
            JSONwriter->StartArray();
            JSONwriter->Uint64(it->first);
            JSONwriter->Uint64(it->second);
            JSONwriter->EndArray();
        }
        JSONwriter->EndArray();
        
        JSONwriter->String("instructionsCDF");
        JSONwriter->StartArray();
        unsigned long long sum = 0;
        it = DistanceDistributionMap.begin();
        for (; it != DistanceDistributionMap.end(); it++) {
            sum += it->second;
            JSONwriter->StartArray();
            JSONwriter->Uint64(it->first);
            JSONwriter->Double(double4((double)sum/norm));
            JSONwriter->EndArray();
        }
        JSONwriter->EndArray();
        
    JSONwriter->EndObject();
    JSONwriter->EndObject();
}

void InstTempReuse::visit(Instruction &I, unsigned long long CurrentIssueCycle) {
    bool found = false;
    unsigned long long distance = 0;
    unsigned long long PreviousIssueCycle = 0;
    unsigned long long CurrentMemoryAccess = 0;
    
    if (this->cache_line_size && this->inst_size) {
        map<Instruction *, unsigned long long>::iterator inst_it;
        int div = this->cache_line_size / this->inst_size;

        inst_it = InstructionsIndex.find(&I);
        CurrentMemoryAccess = inst_it->second / div;
    } else
        CurrentMemoryAccess = (unsigned long long)&I;

    const auto it = LastMemoryAccess.find(CurrentMemoryAccess);
    if (it != LastMemoryAccess.end()) {
        found = 1;
        PreviousIssueCycle = it->second;
        it->second = CurrentIssueCycle;
    }
    else
        LastMemoryAccess.insert(pair<unsigned long long, unsigned long long>(CurrentMemoryAccess, CurrentIssueCycle));

    if (found) {
        DistanceTree = ComputeDistance(PreviousIssueCycle, DistanceTree, &distance);
        if (DistanceTree && DistanceTree->key == PreviousIssueCycle)
            DistanceTree = Update(PreviousIssueCycle, CurrentIssueCycle, DistanceTree);
        else
            DistanceTree = Insert(CurrentIssueCycle, DistanceTree);
    }
    else
        DistanceTree = Insert(CurrentIssueCycle, DistanceTree);

    distance = upperPowerOfTwo(distance);

    if (found) {
        const auto it = DistanceDistributionMap.find(distance);
        if (it == DistanceDistributionMap.end())
            DistanceDistributionMap.insert(pair<unsigned long long, unsigned long long>(distance, 1));
        else {
            unsigned long long prev_attr = it->second;
            it->second = prev_attr + 1;
        }
    }
}
