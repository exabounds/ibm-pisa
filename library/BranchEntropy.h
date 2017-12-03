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

#ifndef LLVM_BRANCH_ENTROPY__H
#define LLVM_BRANCH_ENTROPY__H

#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <pthread.h> 

#include "InstructionAnalysis.h"
#include "utils.h"

class BranchEntropy: public InstructionAnalysis {
    typedef struct {
        int f;
        int bb;
        int i;
    } InstructionID;

    pthread_mutex_t *lock;
    bool only_cond;
    
    // Specifies whether the output file should be sharded.
    bool shard_files_;
    
    // The output stream where the branch entropy is dumped. It can be
    // a file or stdout. The default value is stdout.
    std::unique_ptr<std::ostream> out_;
    
    // The threshold for the maximum entries allowed in a file. This is used
    // only if the output stream is different than stdout. If this threshold
    // is reached and shard_files_ is true, then the files are sharded. The
    // name of a shard is given by the name of the file and the shard number.
    const long int max_file_entries_ = 100000;
    
    // The number of entries currently dumped in the file.
    long int current_file_entries_;
    
    // Output file name. This value should be empty if the output stream is stdout.
    const string filename_;
    
    // The shard number of the output file.
    long shard_;
    
    // Used for buffering debugging information.
    std::string buffer_;
    
    // The maximum size of the debugging buffer. The buffer is logged when
    // the buffer reaches this size.
    const unsigned long max_buffer_size_ = 8192;
    
    BranchInst *prevB = NULL;
    Module *M;
    map<Instruction*, InstructionID> instructionIDs;

    // This function returns (via arguments) the function_id,
    // the basicblock_id and the instruction_id of a given LLVM instruction
    void getInstructionIDs(Instruction *I, int *f, int *bb, int *i, Module *M);

    bool isCTRL(Instruction* I);


public:
    BranchEntropy(Module *M, 
                  pthread_mutex_t *print_lock, 
                  int thread_id, 
                  int processor_id, 
                  bool cond, 
                  const bool shard_files = false, 
                  const string &filename = "");
                  
    ~BranchEntropy();
    void visit(Instruction &I);
    void printHeader();
};

#endif // LLVM_BRANCH_ENTROPY__H

