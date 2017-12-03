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

class InstTempReuse;

#ifndef LLVM_INST_TEMP_REUSE__H
#define LLVM_INST_TEMP_REUSE__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "JSONmanager.h"
#include "splay.h"

class InstTempReuse: public InstructionAnalysis {
    // Cache line size and instruction size
    int cache_line_size;
    int inst_size;

    // Map between LLVM Instructions and indexes
    map<Instruction *, unsigned long long> InstructionsIndex;

    // Map between memory address and last issue cycle access
    map<unsigned long long, unsigned long long> LastMemoryAccess;

    // Splay tree used to compute the number of different memory accesses 
    // since the last access of the current memory address
    tree *DistanceTree;

    // Data structure for storing reuse distance / distribution
    map<unsigned long long, unsigned long long> DistanceDistributionMap;

public:
    InstTempReuse(Module *M, int inst_cache_line_size, int inst_size, int thread_id, int processor_id);
    ~InstTempReuse();

    void visit(Instruction &I, unsigned long long CurrentIssueCycle);
    void JSONdump(JSONmanager *JSONwriter, unsigned long long norm);
};

#endif // LLVM_INST_TEMP_REUSE__H
