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

#include "BranchEntropy.h"

BranchEntropy::BranchEntropy(Module *M, 
                             pthread_mutex_t *print_lock, 
                             int thread_id, 
                             int processor_id, 
                             bool cond, 
                             const bool shard_files, 
                             const string &filename) :
    InstructionAnalysis(M, thread_id, processor_id),
    shard_files_(false), filename_(filename) {
        
    this->lock = print_lock;
    this->only_cond = cond;
    
    if (filename.empty()) {
        out_.reset(&std::cout);
    } else {
        out_.reset(new ofstream(filename, std::ofstream::binary | std::ofstream::out));
    }
    
    this->M = M;
    static pthread_mutex_t instructionIDs_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&instructionIDs_lock);

    if (instructionIDs.empty()) {
        // cerr << "Creating map for branch entropy analysis\n";
        int _f = 0;
        for (Module::iterator F = M->begin(); F != M->end(); F++, _f++) {
            int _bb = 0;
            for (Function::iterator BB = F->begin(); BB != F->end(); BB++, _bb++) {
                int _i = 0;
                for (BasicBlock::iterator Ii = BB->begin(); Ii != BB->end(); Ii++, _i++) {
                    Instruction *J = (Instruction*)Ii;
                    if (isCTRL(J)) {
                        instructionIDs[J].f = _f;
                        instructionIDs[J].bb = _bb;
                        instructionIDs[J].i = _i;
                    }
                }
            }
        }
    }

    pthread_mutex_unlock(&instructionIDs_lock);
}

BranchEntropy::~BranchEntropy() {
    *out_.get() << buffer_;
    
    // Avoid doing a delete on the std::cout reference
    if (out_.get() == &std::cout) {
        out_.release();
    }
}

void BranchEntropy::printHeader() {
    pthread_mutex_lock(this->lock);
    *out_.get() << "| Branch type | Function_ID | BasicBlock_ID | Instruction_ID | T/F | Thread_ID | Processor_ID|\n";
    pthread_mutex_unlock(this->lock);
}

bool BranchEntropy::isCTRL(Instruction* I) {
    unsigned long long OpCode = I->getOpcode();

    switch (OpCode) {
        // Control instructions
        case Instruction::Ret: return true;
        case Instruction::Br: return true;
        case Instruction::Switch: return true;
        case Instruction::IndirectBr: return true;
        case Instruction::Invoke: return true;
        case Instruction::Resume: return true;
        case Instruction::Unreachable: return true;
        case Instruction::Call: return true;
    }

    return false;
}

void BranchEntropy::getInstructionIDs(Instruction *I, int *f, int *bb, int *i, Module *M) {
    if (!M)
        return;

    assert(instructionIDs.find(I) != instructionIDs.end());
    
    *f = instructionIDs[I].f;
    *bb = instructionIDs[I].bb;
    *i = instructionIDs[I].i;
}

void BranchEntropy::visit(Instruction &I) {
    if (I.getOpcode() == Instruction::Br) {
        prevB = cast<BranchInst>(&I);
    } else {
        // If the last analyzed instruction was a branch
        if (prevB) {
            BasicBlock *currentBB = I.getParent();
            if (prevB->getNumSuccessors() > 2) {
                pthread_mutex_lock(this->lock);
                buffer_ += "switch instruction identified in " +
                    std::to_string(thread_id) + " " +
                    std::to_string(processor_id) + "\n";
                    
                if (shard_files_) {
                    current_file_entries_++;
                    if (current_file_entries_ % max_file_entries_ == 0) {
                        current_file_entries_ = 0;
                        shard_++;
                        const string filename = filename_ + std::to_string(shard_);
                        out_.reset(new std::ofstream(filename.c_str()));
                    }
                }
                
                pthread_mutex_unlock(this->lock);
            } else if (!this->only_cond && prevB->getNumSuccessors() < 2) {
                // Unconditional branch
                int f = -1, bb = -1, i = -1;
                getInstructionIDs(prevB, &f, &bb, &i, M);

                pthread_mutex_lock(this->lock);
                buffer_ += "ub " + std::to_string(f) + " " +
                    std::to_string(bb) + " " +
                    std::to_string(i) + " 1 " +
                    std::to_string(thread_id) + " " +
                    std::to_string(processor_id) + "\n";

                if (shard_files_) {
                    current_file_entries_++;
                    if (current_file_entries_ % max_file_entries_ == 0) {
                        current_file_entries_ = 0;
                        shard_++;
                        const string filename = filename_ + std::to_string(shard_);
                        out_.reset(new std::ofstream(filename.c_str()));
                    }
                }

                pthread_mutex_unlock(this->lock);
            } else if (prevB->getNumSuccessors() == 2) {
                int state = -1;
                if (currentBB == prevB->getSuccessor(0))
                    state = 1;
                else if (currentBB == prevB->getSuccessor(1))
                    state = 0;
                
                int f = -1, bb = -1, i = -1;
                getInstructionIDs(prevB, &f, &bb, &i, M);

                pthread_mutex_lock(this->lock);
                buffer_ += "cb " + std::to_string(f) + " " +
                    std::to_string(bb) + " " +
                    std::to_string(i) + " " +
                    std::to_string(state) + " " +
                    std::to_string(thread_id) +
                    " " + std::to_string(processor_id) +
                    "\n";
                    
                if (shard_files_) {
                    current_file_entries_++;
                    if (current_file_entries_ % max_file_entries_ == 0) {
                        current_file_entries_ = 0;
                        shard_++;
                        const string filename = filename_ + std::to_string(shard_);
                        out_.reset(new std::ofstream(filename.c_str()));
                    }
                }

                pthread_mutex_unlock(this->lock); 
            }

            prevB = NULL;
        }

        if (!this->only_cond && I.getOpcode() == Instruction::Ret) {
            // Return instruction
            int f = -1, bb = -1, i = -1;
            getInstructionIDs(&I, &f, &bb, &i, M);

            pthread_mutex_lock(this->lock);
            buffer_ += "rint " + std::to_string(f) + " " +
                std::to_string(bb) + " " + std::to_string(i) +
                " 1 " + std::to_string(thread_id) + " " +
                std::to_string(processor_id) + "\n";
                
            if (shard_files_) {
                current_file_entries_++;
                if (current_file_entries_ % max_file_entries_ == 0) {
                    current_file_entries_ = 0;
                    shard_++;
                    const string filename = filename_ + std::to_string(shard_);
                    out_.reset(new std::ofstream(filename.c_str()));
                }
            }

            pthread_mutex_unlock(this->lock);
        } else if (!this->only_cond && I.getOpcode() == Instruction::Call) {
            // Call  instruction
            int f = -1, bb = -1, i = -1;
            getInstructionIDs(&I, &f, &bb, &i, M);

            pthread_mutex_lock(this->lock);
            buffer_ += "call " + std::to_string(f) + " " +
                std::to_string(bb) + " " + std::to_string(i) +
                " 1 " + std::to_string(thread_id) + " " +
                std::to_string(processor_id) + "\n";
                
            if (shard_files_) {
                current_file_entries_++;
                if (current_file_entries_ % max_file_entries_ == 0) {
                    current_file_entries_ = 0;
                    shard_++;
                    const string filename = filename_ + std::to_string(shard_);
                    out_.reset(new std::ofstream(filename.c_str()));
                }
            }

            pthread_mutex_unlock(this->lock);

            // if the call is to an external function, also add the branch
            // corresponding with the return instruction
            CallInst *call = cast<CallInst>(&I);
            Function *_call = get_calledFunction(call);
            if (_call && _call->begin() == _call->end()) {
                pthread_mutex_lock(this->lock);
                buffer_ += "rext " + std::to_string(f) + " " +
                    std::to_string(bb) + " " +
                    std::to_string(i) +
                    " 1 " + std::to_string(thread_id) +
                    " " + std::to_string(processor_id) +
                    "\n";

                if (shard_files_) {
                    current_file_entries_++;
                    if (current_file_entries_ % max_file_entries_ == 0) {
                        current_file_entries_ = 0;
                        shard_++;
                        const string filename = filename_ + std::to_string(shard_);
                        out_.reset(new std::ofstream(filename.c_str()));
                    }
                }

                pthread_mutex_unlock(this->lock);
            }
        }
    }
    
    pthread_mutex_lock(this->lock);

    if (buffer_.size() >= max_buffer_size_) {
        *out_.get() << buffer_;
        buffer_ = "";
    }

    pthread_mutex_unlock(this->lock);
}
