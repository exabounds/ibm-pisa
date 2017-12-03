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

class MPIdata;

#ifndef LLVM_MPI_DATA_VERBOSE__H
#define LLVM_MPI_DATA_VERBOSE__H

#include "InstructionAnalysis.h"
#include "InstructionMix.h"
#include "utils.h"
#include "MPIcnfSupport.h"
#include "JSONmanager.h"

#include <pthread.h>
#include <list>

enum MPItransferType {
    // Support for MPI point-to-point communication
    mpi_send, mpi_recv, mpi_isend, mpi_irecv, // TODO: sendrecv, isendrecv

    // Support for MPI collective communication
    mpi_bcast, mpi_ibcast, mpi_alltoallv, mpi_alltoall, mpi_ialltoall,
    mpi_gather, mpi_igather, mpi_allgather, mpi_iallgather,
    mpi_reduce, mpi_ireduce, mpi_allreduce, mpi_iallreduce,

    // Support for wait and test MPI calls
    mpi_wait, mpi_test, mpi_waitall, mpi_testall, 

    // Minor support for all the other calls
    mpi_other
};

class MPIdata: public InstructionAnalysis {
    pthread_mutex_t *ls_lock;
    pthread_mutex_t *db_lock;
    MPIdb *db;
    unsigned long long previousCount;
    unsigned long long index;
    unsigned long long sentBytes;
    unsigned long long receivedBytes;
    unsigned long long totalSentMessages;
    unsigned long long totalReceivedMessages;
    list<MPIinfo> async_send, async_recv, async_other;
    /* Map key=sourceID and value=<receivedBytes,numberReceivedMessagesFromSourceID> */
    map<unsigned int, pair <unsigned long long, unsigned long long> > matrixReceived;


    // COLLECTIVE DATA
    
    /*
    FIXME: note that collective routines specify the communicator (i.e. the set of processes taking part to the communication).
           We do not handle the communicator, we always assume MPI_COMM_WORLD .
    */
    
    /*
    call to Bcast IBcast: 
        root: count * sizeof(datatype) -> bcastSentBytes
        notRoot:  count * sizeof(datatype) -> bcastReceivedBytes                    // This send the same data to all processes, counted only once
    call to Alltoall Ialltoall:
        sendcount * sizeof(sendtype) * size(communicator) -> alltoallSentBytes      // Whole sent data. different chunks to different procs
        recvcount * sizeof(recvtype) * size(communicator) -> alltoallReceivedBytes  // Whole recv data. different chunks from different procs
    call to Gather IGather:
        sendcount * sizeof(sendtype) -> gatherSentBytes                                 // All processes, the root send data to itself
        root: recvcount * sizeof(recvtype) * size(communicator) -> gatherReceivedBytes  // Whole recv data, root only
    call to Allgather Iallgather:
        sendcount * sizeof(sendtype) -> allgatherSentBytes                          // All processes, send this amount of data to all other processes
        recvcount * sizeof(recvtype) * size(communicator) -> allgatherReceivedBytes // Whole recv data, all processes
    call to Reduce Ireduce:
        notRoot: count * sizeof(datatype) -> reduceNotRootBytes                     // All processes, contribute to the operation
        root: count * sizeof(datatype) -> reduceRootBytes                           // Root contributes and collect the result
    call to Allreduce Iallreduce:
        count * sizeof(datatype) -> allreduceBytes                                  // All processes, contributes and collect operation result
    */
    
    unsigned long long bcastSentBytes;
    unsigned long long bcastReceivedBytes;

    unsigned long long alltoallSentBytes;
    unsigned long long alltoallReceivedBytes;

    unsigned long long gatherSentBytes;
    unsigned long long gatherReceivedBytes;

    unsigned long long allgatherSentBytes;
    unsigned long long allgatherReceivedBytes;

    unsigned long long reduceRootBytes;
    unsigned long long reduceNotRootBytes;
    unsigned long long allreduceBytes;


public:
    MPIdata(Module *M, pthread_mutex_t *print_lock, int thread_id,
            int processor_id, MPIdb *DB, pthread_mutex_t *db_lock);

    void visit(Instruction *I, struct message *msg);
    void updateReceivedMatrix(unsigned int _source, unsigned long long bytes);
    void updateAsyncReqList(Instruction *I, struct message *msg, unsigned int listType, unsigned long long bytes);
    void JSONdump(JSONmanager *JSONwriter);

protected:
    MPItransferType name2type(const char* cname);
    list<MPIinfo>::iterator getAsyncSend(void* req_addr);  // Returns the iterator it on async_send that repre it->real_addr == req_addr
    list<MPIinfo>::iterator getAsyncRecv(void* req_addr);  // Returns the iterator it on async_recv that repre it->real_addr == req_addr
    list<MPIinfo>::iterator getAsyncOther(void* req_addr); // Returns the iterator it on async_other that repre it->real_addr == req_addr
    void handleRequestCompletion(mpi_request_data msg);
};

#endif // LLVM_MPI_DATA_VERBOSE__H

