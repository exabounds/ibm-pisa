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

#include "MPIdata.h"

MPIdata::MPIdata(Module *M, pthread_mutex_t *print_lock, int thread_id,
                int processor_id, MPIdb *DB, pthread_mutex_t *db_lock)
                : InstructionAnalysis(M, thread_id, processor_id),
    ls_lock(print_lock), db(DB), db_lock(db_lock)
    , async_send(), async_recv(),
    previousCount(0), index(0),
    sentBytes(0), receivedBytes(0),
    totalSentMessages(0), totalReceivedMessages(0),

    bcastSentBytes(0), bcastReceivedBytes(0),
    alltoallSentBytes(0), alltoallReceivedBytes(0),
    gatherSentBytes(0), gatherReceivedBytes(0),
    allgatherSentBytes(0), allgatherReceivedBytes(0),
    reduceRootBytes(0), reduceNotRootBytes(0),
    allreduceBytes(0) {}

void MPIdata::JSONdump(JSONmanager *JSONwriter) {
    JSONwriter->String("mpiDataExchanged");
    JSONwriter->StartObject();

        // Point to point calls data
        JSONwriter->String("sent_bytes");
        JSONwriter->Uint64(sentBytes);

        JSONwriter->String("received_bytes");
        JSONwriter->Uint64(receivedBytes);

        JSONwriter->String("sent_messages");
        JSONwriter->Uint64(totalSentMessages);

        JSONwriter->String("received_messages");
        JSONwriter->Uint64(totalReceivedMessages);
        
        // Collective calls data
        JSONwriter->String("bcast_sent_bytes");
        JSONwriter->Uint64(bcastSentBytes);

        JSONwriter->String("bcast_received_bytes");
        JSONwriter->Uint64(bcastReceivedBytes);


        JSONwriter->String("alltoall_sent_bytes");
        JSONwriter->Uint64(alltoallSentBytes);

        JSONwriter->String("alltoall_received_bytes");
        JSONwriter->Uint64(alltoallReceivedBytes);


        JSONwriter->String("gather_sent_bytes");
        JSONwriter->Uint64(gatherSentBytes);

        JSONwriter->String("gather_received_bytes");
        JSONwriter->Uint64(gatherReceivedBytes);


        JSONwriter->String("allgather_sent_bytes");
        JSONwriter->Uint64(allgatherSentBytes);

        JSONwriter->String("allgather_received_bytes");
        JSONwriter->Uint64(allgatherReceivedBytes);


        JSONwriter->String("reduce_root_bytes");
        JSONwriter->Uint64(reduceRootBytes);

        JSONwriter->String("reduce_not_root_bytes");
        JSONwriter->Uint64(reduceNotRootBytes);

        JSONwriter->String("allreduce_bytes");
        JSONwriter->Uint64(allreduceBytes);

    JSONwriter->EndObject();


    JSONwriter->String("mpiCommunicationVector");
    JSONwriter->StartArray();

    map<unsigned int, pair<unsigned long long, unsigned long long> >::iterator it;
    it = matrixReceived.begin();
    
    for (; it != matrixReceived.end(); it++) {
        JSONwriter->StartObject();
        JSONwriter->String("source");
        JSONwriter->Uint64(it->first);
        JSONwriter->String("received_bytes");
        JSONwriter->Uint64(it->second.first);
        JSONwriter->String("received_messages");
        JSONwriter->Uint64(it->second.second);
        JSONwriter->EndObject();
    }

    JSONwriter->EndArray();
}

void MPIdata::updateReceivedMatrix(unsigned int _source, unsigned long long bytes) {
    assert(_source>=0);

    map<unsigned int, pair<unsigned long long, unsigned long long> >::iterator srcFound;

    srcFound = matrixReceived.find(_source);
    if (srcFound != matrixReceived.end()) {
        srcFound->second.first = srcFound->second.first + bytes;
        srcFound->second.second++;
        return;
    }

    matrixReceived.insert(std::pair<unsigned int, 
    pair <unsigned long long, unsigned long long>>(_source, std::pair<unsigned long long, unsigned long long>(bytes, 1UL)));
}

void MPIdata::updateAsyncReqList(Instruction *I, struct message *msg, unsigned int listType, unsigned long long bytes) {
    MPIinfo info;
    memset(&info.info, 0, sizeof(MPIcall));
    info.info.I = I;
    info.info.msgSize = bytes;

    info.info.tag = msg->data.mpi.tag;
    info.info.comm = msg->data.mpi.comm;
    info.real_addr = msg->data.mpi.req_data.req_addr;

    switch(listType){
        case 1 :/* Irecv call */
            info.info.id_s = msg->data.mpi.other;
            info.info.id_r = processor_id;
        
            async_recv.push_back(info);
            break;
            
            
        case 2 :/* Isend call */
            info.info.id_s = processor_id;
            info.info.id_r = msg->data.mpi.other;
                    
            async_send.push_back(info);
            break;
            
            
        default:/* IDoNotKnow call */
            async_other.push_back(info);
            break;
    }
        

    return;
}

MPItransferType MPIdata::name2type(const char* cname) {
    // Supported MPI point to point communication
    if(!strcasecmp(cname,"MPI_Send"))
        return mpi_send;
    if(!strcasecmp(cname,"MPI_Recv"))
        return mpi_recv;
    if(!strcasecmp(cname,"MPI_Isend"))
        return mpi_isend;
    if(!strcasecmp(cname,"MPI_Irecv"))
        return mpi_irecv;
        
    // Supported wait and test MPI calls
    if(!strcasecmp(cname,"MPI_Wait"))
        return mpi_wait;
    if(!strcasecmp(cname,"MPI_Test"))
        return mpi_test;
    if(!strcasecmp(cname,"MPI_Waitall"))
        return mpi_waitall;
    if(!strcasecmp(cname,"MPI_Testall"))
        return mpi_testall;
        
    // Supported MPI collective communication
    if(!strcasecmp(cname,"MPI_Bcast"))
        return mpi_bcast;
    if(!strcasecmp(cname,"MPI_IBcast"))
        return mpi_ibcast;
    if(!strcasecmp(cname,"MPI_Alltoallv"))
        return mpi_alltoallv;
    if(!strcasecmp(cname,"MPI_Alltoall"))
        return mpi_alltoall;
    if(!strcasecmp(cname,"MPI_Ialltoall"))
        return mpi_ialltoall;
    if(!strcasecmp(cname,"MPI_Gather"))
        return mpi_gather;
    if(!strcasecmp(cname,"MPI_Igather"))
        return mpi_igather;
    if(!strcasecmp(cname,"MPI_Allgather"))
        return mpi_allgather;
    if(!strcasecmp(cname,"MPI_Iallgather"))
        return mpi_iallgather;
    if(!strcasecmp(cname,"MPI_Reduce"))
        return mpi_reduce;
    if(!strcasecmp(cname,"MPI_Ireduce"))
        return mpi_ireduce;
    if(!strcasecmp(cname,"MPI_Allreduce"))
        return mpi_allreduce;
    if(!strcasecmp(cname,"MPI_Iallreduce"))
        return mpi_iallreduce;
        
    // Minor support for all other calls
    return mpi_other;
}

list<MPIinfo>::iterator MPIdata::getAsyncSend(void* req_addr) {
    list<MPIinfo>::iterator it;

    for(it = async_send.begin(); it != async_send.end(); it++) {
        if (it->real_addr == req_addr) {
            break;
        }
    }

    return it;
}

list<MPIinfo>::iterator MPIdata::getAsyncOther(void* req_addr) {
    list<MPIinfo>::iterator it;

    for(it = async_other.begin(); it != async_other.end(); it++) {
        if (it->real_addr == req_addr) {
            break;
        }
    }

    return it;
}

list<MPIinfo>::iterator MPIdata::getAsyncRecv(void* req_addr) {
    list<MPIinfo>::iterator it;

    for(it = async_recv.begin(); it != async_recv.end(); it++) {
        if (it->real_addr == req_addr) {
            break;
        }
    }

    return it;
}

void MPIdata::handleRequestCompletion(mpi_request_data mpi) {
    void* req_addr = mpi.req_addr;
    list<MPIinfo>::iterator it = getAsyncSend(req_addr);

    if ( it != async_send.end() ) {
        sentBytes += it->info.msgSize;
        totalSentMessages++;
        async_send.erase(it);
    } else {
        it = getAsyncRecv(req_addr);

        if(it!=async_recv.end()){
            receivedBytes += it->info.msgSize;
            totalReceivedMessages++;
            updateReceivedMatrix(mpi.source, it->info.msgSize);
            async_recv.erase(it);
        } else {
            it = getAsyncOther(req_addr);
            /* This should not happen */
            // assert(it!=async_other.end());
            
            /*  MPI_Waitall may issue some requests that will be ignored. 
                Those will have source = -1 when waiting for missing MPI_Isend. 
            */
            assert(it!=async_other.end() || mpi.source == -1);
            if(it!=async_other.end())
                async_other.erase(it);
        }
    }
}

void MPIdata::visit(Instruction *I, struct message *msg) {
    assert(msg);

    CallInst *call = cast<CallInst>(I);
    Function *_call = get_calledFunction(call);

    unsigned long long bytes = msg->data.mpi.size * msg->data.mpi.datatype;

    std::string name = _call->getName().str();
    if(name.back() == '_' )
        name.pop_back();
    const char* cname = name.c_str();
    MPItransferType callType = name2type(cname);

    /* MPI statistics - aggregated number of bytes*/
    if(callType == mpi_send){
        sentBytes += bytes;
        totalSentMessages++;
    }
    if(callType ==  mpi_recv){
        receivedBytes += bytes;
        totalReceivedMessages++;
    }

    /* MPI data - communication matrix - received number of bytes per process */
    unsigned int _source;
    switch(callType){

    // Supported MPI point to point communication
        case mpi_send : 
            break;
            
        case mpi_isend : 
            updateAsyncReqList(I, msg, 2, bytes);
            break;
            
        case mpi_recv : 
            _source = msg->data.mpi.other;
            updateReceivedMatrix(_source, bytes);
            break;
            
        case mpi_irecv : 
            updateAsyncReqList(I, msg, 1, bytes);
            break;
            
            
            
    // Supported MPI collective communication
        case mpi_bcast :
            if(msg->data.mpi.root == processor_id)
                bcastSentBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            else
                bcastReceivedBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            break;
            
        case mpi_ibcast :
            if(msg->data.mpi.root == processor_id)
                bcastSentBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            else
                bcastReceivedBytes += msg->data.mpi.size * msg->data.mpi.datatype;
                
            updateAsyncReqList(I, msg, -1, 0);
            break;
            
        case mpi_alltoallv :
            for(int c = 0 ; c < msg->data.mpi.comm_size ; c++){
                alltoallSentBytes += msg->data.mpi.many.sizes[c] * msg->data.mpi.datatype;
                alltoallReceivedBytes += msg->data.mpi.many.opt_recvsizes[c] * msg->data.mpi.opt_recvdatatype;
            }
            break;

        case mpi_alltoall :
            alltoallSentBytes += msg->data.mpi.size * msg->data.mpi.datatype * msg->data.mpi.comm_size;
            alltoallReceivedBytes += msg->data.mpi.opt_recvsize * msg->data.mpi.opt_recvdatatype * msg->data.mpi.comm_size;
            break;

        case mpi_ialltoall :
            alltoallSentBytes += msg->data.mpi.size * msg->data.mpi.datatype * msg->data.mpi.comm_size;
            alltoallReceivedBytes += msg->data.mpi.opt_recvsize * msg->data.mpi.opt_recvdatatype * msg->data.mpi.comm_size;
            
            updateAsyncReqList(I, msg, -1, 0);
            break;

        case mpi_gather :
            gatherSentBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            if(msg->data.mpi.root == processor_id)
                gatherReceivedBytes += msg->data.mpi.opt_recvsize * msg->data.mpi.opt_recvdatatype * msg->data.mpi.comm_size;
            break;

        case mpi_igather :
            gatherSentBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            if(msg->data.mpi.root == processor_id)
                gatherReceivedBytes += msg->data.mpi.opt_recvsize * msg->data.mpi.opt_recvdatatype * msg->data.mpi.comm_size;
            
            updateAsyncReqList(I, msg, -1, 0);
            break;

        case mpi_allgather :
            allgatherSentBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            allgatherReceivedBytes += msg->data.mpi.opt_recvsize * msg->data.mpi.opt_recvdatatype * msg->data.mpi.comm_size;
            break;

        case mpi_iallgather :
            allgatherSentBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            allgatherReceivedBytes += msg->data.mpi.opt_recvsize * msg->data.mpi.opt_recvdatatype * msg->data.mpi.comm_size;
            
            updateAsyncReqList(I, msg, -1, 0);
            break;

        case mpi_reduce :
            if(msg->data.mpi.root == processor_id)
                reduceRootBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            else
                reduceNotRootBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            break;

        case mpi_ireduce :
            if(msg->data.mpi.root == processor_id)
                reduceRootBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            else
                reduceNotRootBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            
            updateAsyncReqList(I, msg, -1, 0);
            break;

        case mpi_allreduce :
            allreduceBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            break;

        case mpi_iallreduce :
            allreduceBytes += msg->data.mpi.size * msg->data.mpi.datatype;
            
            updateAsyncReqList(I, msg, -1, 0);
            break;
            
            
    // Supported wait and test MPI calls
        case mpi_wait : 
            handleRequestCompletion(msg->data.mpi.req_data);
            break;
            
        case mpi_test : 
            if(msg->data.mpi.flag)
                handleRequestCompletion(msg->data.mpi.req_data);
            break;
            
        case mpi_waitall : 
            for(int r = 0 ; r < msg->data.mpi.many.counter ; r++){
                handleRequestCompletion(msg->data.mpi.many.reqs[r]);
            }
            break;
            
        case mpi_testall : 
            if(msg->data.mpi.flag)
                for(int r = 0 ; r < msg->data.mpi.many.counter ; r++){
                    handleRequestCompletion(msg->data.mpi.many.reqs[r]);
                }
            break;
            
    }
}

