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

#ifndef LLVM_LOAD_STORE_VERBOSE__H
#define LLVM_LOAD_STORE_VERBOSE__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "splay.h"

class LoadStoreVerbose: public InstructionAnalysis {
    pthread_mutex_t *lock;
public:
    LoadStoreVerbose(Module *M, pthread_mutex_t *print_lock, int thread_id, int processor_id);
    void visit(Instruction &I, unsigned long long CurrentIssueCycle);
    void printHeader();
};

#endif // LLVM_LOAD_STORE_VERBOSE__H

