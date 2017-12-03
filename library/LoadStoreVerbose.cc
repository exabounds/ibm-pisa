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

#include "LoadStoreVerbose.h"

LoadStoreVerbose::LoadStoreVerbose(Module *M, pthread_mutex_t *print_lock, int thread_id, int processor_id) :
    InstructionAnalysis(M, thread_id, processor_id) {
    this->lock = print_lock;
    this->thread_id = thread_id;
}

void LoadStoreVerbose::printHeader() {
    pthread_mutex_lock(this->lock);
    errs() << "| THREAD_ID | PROCESSOR_ID | CYCLE_NUM | TYPE | ADDRESS | SIZE |\n";
    pthread_mutex_unlock(this->lock);
}

void LoadStoreVerbose::visit(Instruction &I, unsigned long long CurrentIssueCycle) {
    if (I.getOpcode() != Instruction::Load && I.getOpcode() != Instruction::Store)
        return;

    unsigned long long Opcode = I.getOpcode();
    unsigned MemoryAccessSize = getMemoryAccessSize(I);
    void *MemoryAddress = getMemoryAddress(&I, thread_id);

    pthread_mutex_lock(this->lock);
    if (Opcode == Instruction::Load) {
        errs() << thread_id << " " << processor_id << " ";
        errs() << CurrentIssueCycle << " L " << MemoryAddress << " " << MemoryAccessSize << "\n";
    } else if (Opcode == Instruction::Store) {
        errs() << thread_id << " " << processor_id << " ";
        errs() << CurrentIssueCycle << " S " << MemoryAddress << " " << MemoryAccessSize << "\n";
    }
    pthread_mutex_unlock(this->lock);
}


