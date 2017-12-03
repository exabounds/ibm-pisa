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

#include "RegisterCount.h"

RegisterCount::RegisterCount(Module *M, int thread_id, int processor_id) :
    InstructionAnalysis(M, thread_id, processor_id) {
    count_r = 0;
}

void RegisterCount::JSONdump(JSONmanager *JSONwriter) {
    map<Value *, unsigned long long>::iterator it, f;
    unsigned long long count_w = 0;

    for (it = RCwrites.begin(); it != RCwrites.end(); it++) {
        f = RCreads.find(it->first);
        if (f != RCreads.end())
            count_w += it->second;
    }

    JSONwriter->String("registerAccesses");
    JSONwriter->StartObject();

    JSONwriter->String("reads");
    JSONwriter->Uint64(count_r);

    JSONwriter->String("writes");
    JSONwriter->Uint64(count_w);

    JSONwriter->EndObject();
}

void RegisterCount::visit(Instruction &I) {
    const auto it = RCwrites.find(&I);

    if (it != RCwrites.end())
        it->second++;
    else
        RCwrites.insert(pair<Value *, unsigned long long>(&I, 1));

    int NumOperands = I.getNumOperands();
    
    for (int i = 0; i < NumOperands; i++) {
        Value *v = I.getOperand(i);
        if (!dyn_cast<Constant>(v)) {
            count_r++;
            RCreads.insert(pair<Value *, unsigned long long>(v, 0));
        }
    }
}
