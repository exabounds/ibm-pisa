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

class RegisterCount;

#ifndef LLVM_REGSTER_COUNT__H
#define LLVM_REGSTER_COUNT__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "JSONmanager.h"

class RegisterCount: public InstructionAnalysis {
    map<Value *, unsigned long long> RCreads;
    map<Value *, unsigned long long> RCwrites;
    unsigned long long count_r;

public:
    RegisterCount(Module *M, int thread_id, int processor_id);
    void visit(Instruction &I);
    void JSONdump(JSONmanager *JSONwriter);
    void dump();
};

#endif // LLVM_REGSTER_COUNT__H

