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

class MPImap;

#ifndef LLVM_MPI_MAP_VERBOSE__H
#define LLVM_MPI_MAP_VERBOSE__H

#include "InstructionAnalysis.h"
#include "InstructionMix.h"
#include "utils.h"
#include "MPIcnfSupport.h"

#include <list>

class MPImap: public InstructionAnalysis {
    pthread_mutex_t *ls_lock;
    pthread_mutex_t *db_lock;
    MPIdb *db;
    std::unique_ptr<InstructionMix> *mix;
    list<MPIinfo> async_send, async_recv;
    unsigned long long previousCount;
    unsigned long long index;
    int inFunction;
    
    // This variable is used for asynchronous communication with MPI_Test.
    // If mode is 1 then all the instructions between two consecutive failed Tests are removed from the counters.
    int mode;
    
    int tmpDelta;
    int accMode;

public:
    MPImap(Module *M, 
           pthread_mutex_t *print_lock, 
           int thread_id,
           int processor_id, 
           MPIdb *DB, 
           pthread_mutex_t *db_lock, 
           std::unique_ptr<InstructionMix> *mix, 
           int accMode);

    void extract_MPI_Send(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Recv(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Isend(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Irecv(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Wait(MPIcall *, char *, struct mpi_call_notification_msg *);
    void extract_MPI_Bcast(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Reduce(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Allreduce(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Scatter(MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Sendrecv(MPIcall *, MPIcall *, struct mpi_call_notification_msg *);
    void extract_MPI_Init(MPIcall *);
    void extract_MPI_Finalize(MPIcall *);

    void extract_info(string call_name, 
                      MPIcall *update, 
                      MPIcall *update2, 
                      struct mpi_call_notification_msg *msg);

    void send_msg_to_db(MPIcall *msg);
    void remove_from_db(struct MPIcall_node **sr, struct MPIcall_node *v);
    void recv_msg_from_db(MPIcall *msg);
    void send_msg_to_all_comm(MPIcall *update, int size);
    void send_query_and_update(MPIcall *update);
    void send_query_and_update_comm(MPIcall *update, int size);

    struct MPIcall_node * create_new_MPIcall_node(MPIcall *info);
    int items_are_equal(MPIcall *a, MPIcall *b);

    void update_MPI_Isend(MPIcall *update);
    void update_MPI_Irecv(MPIcall *update);
    void update_MPI_Send(MPIcall *update, struct mpi_call_notification_msg *msg);
    void update_MPI_Recv(MPIcall *update, struct mpi_call_notification_msg *msg);
    void update_MPI_Sendrecv(MPIcall *update, MPIcall *update2, struct mpi_call_notification_msg *msg);
    void update_MPI_Wait(MPIcall *update, struct mpi_call_notification_msg *msg);
    void update_MPI_Bcast(MPIcall *update, struct mpi_call_notification_msg *msg);
    void update_MPI_Reduce(MPIcall *update, struct mpi_call_notification_msg *msg);
    void update_MPI_Allreduce(MPIcall *update, struct mpi_call_notification_msg *msg);
    void update_MPI_Scatter(MPIcall *update, struct mpi_call_notification_msg *msg);
    void update_MPI_Init(MPIcall *);
    void update_MPI_Finalize(MPIcall *);

    void update_info(string call_name, 
                     MPIcall *update, 
                     MPIcall *update2, 
                     struct mpi_call_notification_msg *msg);
                     
    void visit(Instruction *I, struct message *msg, int tag, int inFunction);
    void printHeader();
};

#endif // LLVM_MPI_MAP_VERBOSE__H

