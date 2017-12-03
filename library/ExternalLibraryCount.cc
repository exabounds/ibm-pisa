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

#include "ExternalLibraryCount.h"

ExternalLibraryCount::ExternalLibraryCount(Module *M, int thread_id, int processor_id) :
    InstructionAnalysis(M, thread_id, processor_id) {
}

bool ExternalLibraryCount::caseInsensitiveOperator::caseInsensitiveOperator::operator()(const std::string & s1, 
                                                                                        const std::string & s2) const {
    int res;
    res = strcasecmp(s1.c_str(),s2.c_str());

    return res<0;
}

void ExternalLibraryCount::JSONdump(JSONmanager *JSONwriter) {
    map<string, unsigned long long>::iterator it;

    JSONwriter->String("externalLibraryCallCount");
    JSONwriter->StartObject();


    for (it = callCount.begin(); it != callCount.end(); it++) {
        JSONwriter->String( it->first.c_str() );
        JSONwriter->Uint64(it->second);
    }

    JSONwriter->EndObject();
}

void ExternalLibraryCount::visit(Instruction &I) {
    if (!strcmp(I.getOpcodeName(), "call")) {
        CallInst *call = cast<CallInst>(&I);
        Function *_call = get_calledFunction(call);
        
        if (!_call) { 
            Value * a = (Function *)call->getCalledValue();
            void *addr = getMemoryAddress((Instruction *)a, thread_id);
            if (addr)
                addr = (void *)*(unsigned long long *)addr;
            _call = getFunctionAddress(addr);
        }
        
        if(!_call)
            return;
              
        // Internal function
        if (_call && _call->begin() != _call->end())
            return;
        
        // Call to an external function.
        map<string, unsigned long long,caseInsensitiveOperator>::iterator it;
        it = callCount.find(_call->getName().str());
        if(it != callCount.end())
            (it->second)++;
        else
            callCount[_call->getName().str()] = 1;
    }
}
