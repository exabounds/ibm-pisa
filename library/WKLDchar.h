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

class WKLDchar;

#ifndef LLVM_WKLD_CHAR_H_
#define LLVM_WKLD_CHAR_H_

#include <memory>
#include "utils.h"

#include "llvm/IR/DataLayout.h"
#include "InstructionAnalysis.h"
#include "InstructionMix.h"
#include "DataTempReuse.h"
#include "InstTempReuse.h"
#include "RegisterCount.h"
#include "LoadStoreVerbose.h"
#include "BranchEntropy.h"
#include "MPIstats.h"
#include "MPImap.h"
#include "MPIdata.h"
#include "ILP.h"
#include "OpenMPstats.h"
#include "ExternalLibraryCount.h"
#include "JSONmanager.h"

class WKLDchar {
    Module *M;
    int flags;

public:
    int thread_id;
    int processor_id;

    // Analysis pointers
    std::unique_ptr<InstructionMix> mix;
    std::unique_ptr<DataTempReuse>  dtr;
    std::unique_ptr<InstTempReuse>  itr;
    std::unique_ptr<RegisterCount>  rc;
    std::unique_ptr<LoadStoreVerbose> ls;
    std::unique_ptr<BranchEntropy>  be;
    std::unique_ptr<MPIstats>   mpi_stats;
    std::unique_ptr<MPImap>     mpi_map;
    std::unique_ptr<MPIdata>    mpi_data;
    std::unique_ptr<ILP> ilp;
    std::unique_ptr<OpenMPstats> openmp_stats;
    std::unique_ptr<ExternalLibraryCount> elc;

    // Coupled constructor
    WKLDchar(Module *M, 
             unsigned long long flags, 
             int data_cache_line_size, 
             int data_reuse_distance_resolution,
             int data_reuse_distance_resolution_final_bin, 
             int inst_cache_line_size,
             int inst_size, 
             int ilp_type, 
             int debug_flag, 
             int window_size,
             int thread_id, 
             int processor_id, 
             pthread_mutex_t *print_lock,
             pthread_mutex_t *output_lock, 
             char *branch_entropy_file);

    WKLDchar(){};

    // Decoupled constructor
    WKLDchar(Module *M, 
             unsigned long long flags, 
             int data_cache_line_size, 
             int data_reuse_distance_distribution,
             int data_reuse_distance_distribution_final_bin, 
             int inst_cache_line_size,
             int inst_size, 
             int ilp_type, 
             int debug_flag, 
             int window_size,
             int thread_id, 
             int processor_id, 
             pthread_mutex_t *print_lock,
             MPIdb *DB, 
             pthread_mutex_t *db_lock,
             int accMode, 
             pthread_mutex_t* output_lock, /* This protects a critical section that only prints debugging messages in ILP. */
             char *branch_entropy_file);


    void analyze(Instruction &I);
    void process_mpi_map(Instruction *I, struct message *msg, int tag, int inFunction);
    void process_mpi_data(Instruction *I, struct message *msg);
    void JSONdump(JSONmanager *JSONwriter, unsigned long long sharedBytes, unsigned long long sharedAccesses);

    // The processor_id for MPI tests can't be extracted before MPI_Init
    // is called. Because we can't be sure when this function is called,
    // we extract the processor_id just before MPI_Finalize is call. In
    // order to have a correct dump, we need to update the value among
    // every class. This is also the reason why every analysis receives
    // a pointer to processor_id from this class.
    void update_processor_id(int processor_id);

    unsigned long long getInstCount();
    void updateILPforCall(Value *returnInstruction, Value *callInstruction);
    void updateILPIssueCycle(int type, Value *I, void *real_addr, unsigned long long issue_cycle);
    
};

#endif
