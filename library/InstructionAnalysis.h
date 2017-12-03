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

#ifndef LLVM_INST_ANALYSIS_H
#define LLVM_INST_ANALYSIS_H

#if LLVM_VERSION_MINOR > 4
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/DebugInfo.h"
#else
#include "llvm/InstVisitor.h"
#include "llvm/DebugInfo.h"
#endif
#include "llvm/Support/Debug.h"

#include <map>
#include <memory>
#include <iostream>

#include <mpi.h>
#include <omp.h>
#include <pthread.h>
#include "llvm/IR/DataLayout.h"

#define CTRL_TYPE       0
#define INT_ADD_TYPE    1
#define INT_SUB_TYPE    2
#define INT_MUL_TYPE    3
#define INT_UDIV_TYPE   4
#define INT_SDIV_TYPE   5
#define INT_UREM_TYPE   6
#define INT_SREM_TYPE   7
#define FP_ADD_TYPE     8
#define FP_SUB_TYPE     9
#define FP_MUL_TYPE     10
#define FP_DIV_TYPE     11
#define FP_REM_TYPE     12
#define LD_FP_16BITS_TYPE   13
#define LD_FP_32BITS_TYPE   14
#define LD_FP_64BITS_TYPE   15
#define LD_FP_80BITS_TYPE   16
#define LD_FP_128BITS_TYPE  17
#define LD_INT_4BITS_TYPE   18 
#define LD_INT_8BITS_TYPE   19
#define LD_INT_16BITS_TYPE  20
#define LD_INT_32BITS_TYPE  21
#define LD_INT_64BITS_TYPE  22
#define STORE_FP_16BITS_TYPE    23
#define STORE_FP_32BITS_TYPE    24
#define STORE_FP_64BITS_TYPE    25
#define STORE_FP_80BITS_TYPE    26
#define STORE_FP_128BITS_TYPE   27
#define STORE_INT_4BITS_TYPE    28 
#define STORE_INT_8BITS_TYPE    29
#define STORE_INT_16BITS_TYPE   30
#define STORE_INT_32BITS_TYPE   31
#define STORE_INT_64BITS_TYPE   32
#define MISC_MEM_TYPE   33
#define BITWISE_TYPE    34  
#define CONV_TYPE       35
#define MISC_TYPE       36

#define INT_CMP_TYPE    40
#define FP_CMP_TYPE     41
#define PHI_TYPE        42
#define ATOMIC_MEM_TYPE 43
#define ARITH_ADDRESS_TYPE  44
#define MISC_VECTOR_TYPE    45
#define MISC_AGGREGATE_TYPE 46
#define SYNC_TYPE       47

#define N_INST_TYPES  37
#define N_BYTES_INST  4

using namespace llvm;
using namespace std;

class  InstructionAnalysis {
protected:
    Module *M;
    const std::unique_ptr<DataLayout> ld;

public:
    int processor_id;
    int thread_id;
    
    InstructionAnalysis(Module *M, int thread_id, int processor_id);
    int getInstType(Instruction &I);
    bool isOfIntegerType(Instruction &I);
    bool isOfMemoryType(Instruction &I);
    bool isOfFloatingPointType(Instruction &I);
    bool isOfControlType(Instruction &I);
    int getMemoryInstType (Instruction &I);
    unsigned getMemoryAccessSize(Instruction &I);
};

#endif
