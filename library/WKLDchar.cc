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

#include "WKLDchar.h"
#include "utils.h"

// Coupled PISA constructor
WKLDchar::WKLDchar(Module *M, 
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
                   char *branch_entropy_file) {
    this->M = M;
    this->flags = flags;
    this->thread_id = thread_id;
    this->processor_id = processor_id;
    this->mix.reset(new InstructionMix(M, thread_id, this->processor_id));

    // DIMEMAS compatibility: print MPI trace in DIMEMAS format
    if (flags & ANALYZE_MPI_MAP) {
        this->mpi_map.reset(new MPImap(M, 
                                       print_lock, 
                                       thread_id, 
                                       this->processor_id, 
                                       NULL/*DB*/, 
                                       NULL/*db_lock*/, 
                                       &this->mix, 0/*accMode*/));
        this->mpi_map->printHeader();
    } else {
        this->mpi_map = NULL;
    }

    // MPI traffic matrix analysis
    if (flags & ANALYZE_MPI_DATA) {
        this->mpi_data.reset(new MPIdata(M, 
                                         print_lock, 
                                         thread_id, 
                                         this->processor_id, 
                                         NULL/*DB*/, 
                                         NULL/*db_lock*/));
    } else {
        this->mpi_data = NULL;
    }

    // Data temporal reuse analysis
    if (flags & ANALYZE_DTR) {
        if (flags & ANALYZE_MEM_FOOTPRINT)
            this->dtr.reset(new DataTempReuse(M, 
                                              data_cache_line_size, 
                                              data_reuse_distance_resolution,
                                              data_reuse_distance_resolution_final_bin, 
                                              thread_id, 
                                              this->processor_id, 
                                              1, 
                                              print_lock));
        else
            this->dtr.reset(new DataTempReuse(M, 
                                              data_cache_line_size, 
                                              data_reuse_distance_resolution,
                                              data_reuse_distance_resolution_final_bin, 
                                              thread_id, 
                                              this->processor_id, 
                                              0, 
                                              print_lock));
    }

    // Instruction temporal reuse analysis
    if (flags & ANALYZE_ITR)
        this->itr.reset(new InstTempReuse(M, 
                                          inst_cache_line_size, 
                                          inst_size, 
                                          thread_id, 
                                          this->processor_id));

    // Register counting analysis
    if (flags & ANALYZE_REG_COUNT)
        this->rc.reset(new RegisterCount(M, 
                                         thread_id, 
                                         this->processor_id));

    // Load store verbose trace
    if (flags & PRINT_LOAD_STORE) {
        this->ls.reset(new LoadStoreVerbose(M, 
                                            print_lock, 
                                            thread_id, 
                                            this->processor_id));
        this->ls->printHeader();
    }

    // Branch trace
    if (flags & PRINT_BRANCH) {
        this->be.reset(new BranchEntropy(M, 
                                         print_lock, 
                                         thread_id, 
                                         this->processor_id, 
                                         false, 
                                         false, 
                                         branch_entropy_file));
        this->be->printHeader();
    } else if (flags & PRINT_BRANCH_COND) {
        this->be.reset(new BranchEntropy(M, 
                                         print_lock, 
                                         thread_id, 
                                         this->processor_id, 
                                         false, 
                                         false, 
                                         branch_entropy_file));
        this->be->printHeader();
    }

    // MPI calls analysis
    if (flags & ANALYZE_MPI_CALLS)
        this->mpi_stats.reset(new MPIstats(M, 
                                           thread_id, 
                                           this->processor_id));

    // Instruction level parallelistm analysis
    if (flags & ANALYZE_ILP)
        this->ilp.reset(new ILP(M, 
                                thread_id, 
                                this->processor_id, 
                                flags, 
                                ilp_type, 
                                window_size, 
                                debug_flag, 
                                output_lock));

    // OpenMP calls analysis
    if (flags & ANALYZE_OPENMP_CALLS)
        this->openmp_stats.reset(new OpenMPstats(M, thread_id, this->processor_id));

    // External function calls counting analysis
    if (flags & ANALYZE_EXTERNALLIBS_CALLS)
        this->elc.reset(new ExternalLibraryCount(M, thread_id, this->processor_id));
}

// Decoupled PISA constructor
WKLDchar::WKLDchar(Module *M, 
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
                   MPIdb *DB, 
                   pthread_mutex_t *db_lock,
                   int accMode,
                   pthread_mutex_t* output_lock, 
                   char *branch_entropy_file) {
    this->M = M;
    this->flags = flags;

    this->thread_id = thread_id;
    this->processor_id = processor_id;


    this->mix.reset(new InstructionMix(M, thread_id, this->processor_id));

    if (flags & ANALYZE_MPI_MAP) {
        this->mpi_map.reset(new MPImap(M, 
                                       print_lock, 
                                       thread_id, 
                                       this->processor_id, 
                                       DB, 
                                       db_lock, 
                                       &this->mix, accMode));
        this->mpi_map->printHeader();
    } else
        this->mpi_map = NULL;

        if (flags & ANALYZE_MPI_DATA) {
            this->mpi_data.reset(new MPIdata(M, 
                                             print_lock, 
                                             thread_id, 
                                             this->processor_id, 
                                             DB, 
                                             db_lock));
        } else
            this->mpi_data = NULL;


    // data temporal reuse analysis
    if (flags & ANALYZE_DTR) {
        if (flags & ANALYZE_MEM_FOOTPRINT)
            this->dtr.reset(new DataTempReuse(M, 
                                              data_cache_line_size, 
                                              data_reuse_distance_resolution,
                                              data_reuse_distance_resolution_final_bin, 
                                              thread_id, 
                                              this->processor_id, 
                                              1, 
                                              print_lock));
        else
            this->dtr.reset(new DataTempReuse(M, 
                                              data_cache_line_size, 
                                              data_reuse_distance_resolution,
                                              data_reuse_distance_resolution_final_bin, 
                                              thread_id, 
                                              this->processor_id, 
                                              0, 
                                              print_lock));
    }

    // instruction temporal reuse analysis
    if (flags & ANALYZE_ITR)
        this->itr.reset(new InstTempReuse(M, 
                                          inst_cache_line_size, 
                                          inst_size, 
                                          thread_id, 
                                          this->processor_id));

    // register counting
    if (flags & ANALYZE_REG_COUNT)
        this->rc.reset(new RegisterCount(M, 
                                         thread_id, 
                                         this->processor_id));

    // load store verbose information
    if (flags & PRINT_LOAD_STORE) {
        this->ls.reset(new LoadStoreVerbose(M, 
                                            print_lock, 
                                            thread_id, 
                                            this->processor_id));
        this->ls->printHeader();
    }

    if (flags & PRINT_BRANCH) {
        this->be.reset(new BranchEntropy(M, 
                                         print_lock, 
                                         thread_id, 
                                         this->processor_id, 
                                         false, 
                                         false, 
                                         branch_entropy_file));
        this->be->printHeader();
    } else if (flags & PRINT_BRANCH_COND) {
        this->be.reset(new BranchEntropy(M, 
                                         print_lock, 
                                         thread_id, 
                                         this->processor_id, 
                                         false, 
                                         false, 
                                         branch_entropy_file));
        this->be->printHeader();
    }
    // MPI calls analysis
    if (flags & ANALYZE_MPI_CALLS)
        this->mpi_stats.reset(new MPIstats(M, 
                                           thread_id, 
                                           this->processor_id));

    // instruction level parallelistm analysis
    if (flags & ANALYZE_ILP)
        this->ilp.reset(new ILP(M, 
                                thread_id, 
                                this->processor_id, 
                                flags, 
                                ilp_type, 
                                window_size, 
                                debug_flag, 
                                output_lock));

    // OpenMP calls analysis
    if (flags & ANALYZE_OPENMP_CALLS)
        this->openmp_stats.reset(new OpenMPstats(M, thread_id, this->processor_id));
                
    // ExternalLibraryCount calls analysis
    if (flags & ANALYZE_EXTERNALLIBS_CALLS)
        this->elc.reset(new ExternalLibraryCount(M, thread_id, this->processor_id));
}

void WKLDchar::analyze(Instruction &I) {
    // Instruction Level Parallelism Analysis
    // This needs to be done before instruction mix
    if (this->ilp) this->ilp->visit(I);

    if (isa<PHINode>(I)) return;

    // Update instruction mix counters
    this->mix->visit(I);
    this->mix->visitMemoryInstruction(I);
    this->mix->visitBinaryArithBitwiseCmpConvertSelectInstruction(I);
    
    unsigned long long CurrentIssueCycle = this->mix->getNumTotalInsts();

    // data temporal reuse analysis
    if (this->dtr) 
        this->dtr->visit(I, CurrentIssueCycle);

    // instruction temporal reuse analysis
    if (this->itr) 
        this->itr->visit(I, CurrentIssueCycle);

    // register counting
    if (this->rc) 
        this->rc->visit(I);

    // print load store verbose information
    if (this->ls) 
        this->ls->visit(I, CurrentIssueCycle);

    // print branch information
    if (this->be) 
        this->be->visit(I);

    // MPI calls analysis
    if (this->mpi_stats) 
        this->mpi_stats->visit(I);

    // OpenMP calls analysis
    if (this->openmp_stats) 
        this->openmp_stats->visit(I);
    
    // External library calls analysis
    if (this->elc)
        this->elc->visit(I);

}

void WKLDchar::process_mpi_map(Instruction *I, struct message *msg, int tag, int inFunction) {
    if (this->mpi_map)
        this->mpi_map->visit(I, msg, tag, inFunction);
}

void WKLDchar::process_mpi_data(Instruction *I, struct message *msg) {
    if (this->mpi_data)
        this->mpi_data->visit(I, msg);
}

unsigned long long WKLDchar::getInstCount() {
    return this->mix->getNumTotalInsts();
}

void WKLDchar::JSONdump(JSONmanager *JSONwriter, unsigned long long sharedBytes, unsigned long long sharedAccesses) {

    JSONwriter->StartObject();

    JSONwriter->String("threadId");
    JSONwriter->Uint64(thread_id);

    JSONwriter->String("processId");
    JSONwriter->Uint64(processor_id);
    
    this->mix->JSONdump_vectorIncluded(JSONwriter);

    if (this->openmp_stats) 
        this->openmp_stats->JSONdump(JSONwriter);
        
    if (this->mpi_stats)
        this->mpi_stats->JSONdump(JSONwriter);

    if (this->ilp)
        this->ilp->JSONdump(JSONwriter, this->mix.get());

    if (this->dtr) {
        JSONwriter->String("dataReuseDistribution");

        if (this->dtr) {
            unsigned long long norm = this->mix->getNumLoadInst() + this->mix->getNumStoreInst();
            this->dtr->JSONdump(JSONwriter, norm, sharedBytes, sharedAccesses);
        }
    }

    if (this->itr) {
        JSONwriter->String("instReuseDistribution");
        JSONwriter->StartArray();

        if (this->itr) {
            unsigned long long norm = this->mix->getNumTotalInsts();
            this->itr->JSONdump(JSONwriter, norm);
        }

        JSONwriter->EndArray();
    }
    
    if (this->rc)
        this->rc->JSONdump(JSONwriter);

    if (this->mpi_data)
        this->mpi_data->JSONdump(JSONwriter);

    if (this->elc)
        this->elc->JSONdump(JSONwriter);

    JSONwriter->EndObject();
}

void WKLDchar::update_processor_id(int processor_id) {
    this->processor_id = processor_id;

    if (ilp)
        ilp->processor_id = processor_id;
    if (mix) 
        mix->processor_id = processor_id;
    if (dtr)
        dtr->processor_id = processor_id;
    if (itr)
        itr->processor_id = processor_id;
    if (rc) 
        rc->processor_id = processor_id;
    if (ls)
        ls->processor_id = processor_id;
    if (be) 
        be->processor_id = processor_id;
    if (mpi_stats) 
        mpi_stats->processor_id = processor_id;
    if (mpi_map) 
        mpi_map->processor_id = processor_id;
    if (mpi_data) 
        mpi_data->processor_id = processor_id;
    if (openmp_stats) 
        openmp_stats->processor_id = processor_id;
    if (elc) 
        elc->processor_id = processor_id;

    // FIXME: this is not gonna work for MPI+OpenMP applications. Either MPI or OpenMP!
    if(weDecoupled){
        if (ilp)
            ilp->thread_id = processor_id;
        if (mix)
            mix->thread_id = processor_id;
        if (dtr) 
            dtr->thread_id = processor_id;
        if (itr)
            itr->thread_id = processor_id;
        if (rc)
            rc->thread_id = processor_id;
        if (ls)
            ls->thread_id = processor_id;
        if (be) 
            be->thread_id = processor_id;
        if (mpi_stats)
            mpi_stats->thread_id = processor_id;
        if (mpi_map) 
            mpi_map->thread_id = processor_id;
        if (mpi_data)
            mpi_data->thread_id = processor_id;
        if (openmp_stats)
            openmp_stats->thread_id = processor_id;
        if (elc)
            elc->thread_id = processor_id;
    }
}

void WKLDchar::updateILPforCall(Value *returnInstruction, Value *callInstruction) {
    if (this->ilp)
        this->ilp->updateILPforCall(returnInstruction, callInstruction);
}

void WKLDchar::updateILPIssueCycle(int type, Value *I, void *real_addr, unsigned long long issue_cycle) {
    if (this->ilp)
        this->ilp->updateILPIssueCycle(type, I, real_addr, issue_cycle);
}
