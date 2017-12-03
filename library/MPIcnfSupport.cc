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

#include "MPIcnfSupport.h"

extern "C" void mpi_update_db(int f, int bb, int i, int isFortran, ...) {
    // This is set to true if we need to clean the memory of the message:
    //    waitall, waitsome, waitany, testall, testsome, testany
    bool cleanMem = false;
    
    struct message msg;
    MPI_Status *ptr;
    memset(&msg, 0, sizeof(msg));
    msg.type = MPI_CALL_NOTIFICATION;

    msg.data.mpi.f_id = f;
    msg.data.mpi.bb_id = bb;
    msg.data.mpi.i_id = i;

    va_list argp;
    va_start(argp, isFortran);

    int type = va_arg(argp, int);

    switch(type) {
    case BASIC_MPI_CALL: {
        msg.data.mpi.other = GET_INT;
        msg.data.mpi.tag = GET_INT;
        msg.data.mpi.comm = GET_COMM;
        msg.data.mpi.size = GET_INT;
        MPI_Type_size(GET_TYPE,&(msg.data.mpi.datatype));
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);
        } break;
    case ASYNC_MPI_CALL: {
        msg.data.mpi.other = GET_INT;
        msg.data.mpi.tag = GET_INT;
        msg.data.mpi.comm = GET_COMM;
        msg.data.mpi.req_data.req_addr = GET_REQADDR;
    msg.data.mpi.size = GET_INT;
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);
        } break;
    case WAIT_MPI_CALL: {
        msg.data.mpi.req_data.req_addr = GET_REQADDR;
        void* ptr_temp = va_arg(argp, void *);
        msg.data.mpi.req_data.source = -1;

        // What if ptr_temp is MPI_STATUS_IGNORE?
        assert(ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE);
        if (ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE) {
            CONVERT_STATUS_POINTER(ptr_temp,ptr);
        
            assert(ptr);
            msg.data.mpi.req_data.source = ptr->MPI_SOURCE;
            msg.data.mpi.flag = 1;
            
            // if source is -1, the request is issued but ignored.
            // assert(ptr->MPI_SOURCE!=-1); 
        }
    } break;

    case WAITALL_MPI_CALL: {
        msg.data.mpi.many.counter = GET_INT;
        //assert(msg.data.mpi.many.counter>0);
        
        cleanMem = true;
        msg.data.mpi.many.reqs = new mpi_request_data[msg.data.mpi.many.counter];
        
        assert(msg.data.mpi.many.reqs != NULL);
        
        // FORTRAN: ["counter" x i32]* : a pointer to a counter-long array of integer (MPI_Fint I assume)
        // C: struct.ompi_request_t** : a pointer to a list of requests (addresses)
        void* reqList = GET_REQADDR;
        
        // FORTRAN: [("counter"*MPI_F_STATUS_SIZE) x i32]* : a pointer to an array of integer
        // C: struct.ompi_status_public_t* : a pointer to a status
        void* statusList = va_arg(argp, void *);
        
        for(int r = 0 ; r < msg.data.mpi.many.counter ; r++){
        
            msg.data.mpi.many.reqs[r].req_addr = reqList;
            void* ptr_temp = statusList;
            
            // What if ptr_temp is MPI_STATUS_IGNORE?
            assert(ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE);
            if (ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE) {
                CONVERT_STATUS_POINTER(ptr_temp,ptr);
            
                assert(ptr);
                msg.data.mpi.many.reqs[r].source = ptr->MPI_SOURCE;
                msg.data.mpi.flag = 1;
                // assert(ptr->MPI_SOURCE!=-1); /* If source is -1, the request is issued but ignored! */
            }
            
            if(isFortran)
                // TODO: In Fortran we move to the next conter-long array [12 x i32]*
                reqList = (void*) (((MPI_Fint*)reqList)+1);
            else
                // Next pointer to address destination: struct.ompi_request_t**
                reqList = (void*) (((MPI_Request**)reqList)+1);
                // Next pointer to status array: [72 x i32]* -> for an MPI_Wait, req is of type: [6 x i32]*
                // FIXME: (int)(sizeof(MPI_Status) / sizeof(int)) is taken from: status_f2c.c:77 
                // from openmpi-1.8.6 . It looks quite implementation agnostic but it might be buggy.
            if(isFortran)
                statusList = (void*) (((MPI_Fint*)statusList) + (int)(sizeof(MPI_Status) / sizeof(MPI_Fint)) );
            else
                statusList = (void*) (((MPI_Status*)statusList)+1);// Next pointer to status array: struct.ompi_status_public_t*
        }
        
    } break;
        
    case TEST_MPI_CALL: {
        msg.data.mpi.req_data.req_addr = GET_REQADDR;
        
        // In C it was dereferencing and in* here. 
        // But the FORTRAN parameter passing should be fine as it is.
        msg.data.mpi.flag = GET_INT_FROM_POINTER;
        void* ptr_temp = va_arg(argp, void *);
        msg.data.mpi.req_data.source = -1;

        // What if ptr_temp is MPI_STATUS_IGNORE?
        // assert(ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE);
        if (ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE) {
            CONVERT_STATUS_POINTER(ptr_temp,ptr);
        
            assert(ptr);
            msg.data.mpi.req_data.source = ptr->MPI_SOURCE;
            if (msg.data.mpi.flag && ptr->MPI_SOURCE==-1)
                assert(ptr->MPI_SOURCE!=-1);
        }
    } break;
        
    case TESTALL_MPI_CALL: {
        msg.data.mpi.many.counter = GET_INT;
        //assert(msg.data.mpi.many.counter>0);
        
        cleanMem = true;
        msg.data.mpi.many.reqs = new mpi_request_data[msg.data.mpi.many.counter];
        
        assert(msg.data.mpi.many.reqs != NULL);
        
        // FORTRAN: ["counter" x i32]* : a pointer to a counter-long array of integer (MPI_Fint I assume)
        // C: struct.ompi_request_t** : a pointer to a list of requests (addresses)
        void* reqList = GET_REQADDR;
        
        // In C it was dereferencing and in* here. 
        // But the FORTRAN parameter passing should be fine as it is.
        msg.data.mpi.flag = GET_INT_FROM_POINTER;
        
        // FORTRAN: [("counter"*MPI_F_STATUS_SIZE) x i32]* : a pointer to an array of integer
        // C: struct.ompi_status_public_t* : a pointer to a status
        void* statusList = va_arg(argp, void *);
        
            
        for(int r = 0 ; r < msg.data.mpi.many.counter ; r++){
        
            msg.data.mpi.many.reqs[r].req_addr = reqList;
            void* ptr_temp = statusList;
            
            // What if ptr_temp is MPI_STATUS_IGNORE?
            assert(ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE);
            if (ptr_temp != MPI_STATUS_IGNORE && ptr_temp != MPI_F_STATUS_IGNORE) {
                CONVERT_STATUS_POINTER(ptr_temp,ptr);
                
                assert(ptr);
                msg.data.mpi.many.reqs[r].source = ptr->MPI_SOURCE;    
                if (msg.data.mpi.flag && ptr->MPI_SOURCE==-1)
                    assert(ptr->MPI_SOURCE!=-1);
            }
            
            if(isFortran)
                // TODO: In Fortran we move to the next conter-long array [12 x i32]*
                reqList = (void*) (((MPI_Fint*)reqList)+1);
            else
                // Next pointer to address destination: struct.ompi_request_t**
                reqList = (void*) (((MPI_Request**)reqList)+1);
            
            //  Next pointer to status array: [72 x i32]* -> for an MPI_Wait, req is of type: [6 x i32]*
            //  FIXME: (int)(sizeof(MPI_Status) / sizeof(int)) is taken from: status_f2c.c:77 
            //  from openmpi-1.8.6 . It looks quite implementation agnostic but it might be buggy.
            if(isFortran)
                statusList = (void*) (((MPI_Fint*)statusList) + (int)(sizeof(MPI_Status) / sizeof(MPI_Fint)) );
            else
                // Next pointer to status array: struct.ompi_status_public_t*
                statusList = (void*) (((MPI_Status*)statusList)+1);
        }
    } break;
        
    case ANYTAG_MPI_CALL: {
        msg.data.mpi.other = GET_INT;
        msg.data.mpi.comm = GET_COMM;
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);
    } break;
    
    case ALL_MPI_CALL: {
        // Currently tested on AllReduce calls only
        msg.data.mpi.comm = GET_COMM;
        msg.data.mpi.size = GET_INT;
        MPI_Type_size(GET_TYPE,&(msg.data.mpi.datatype));
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);
    } break;
    
    case SENDRECV_MPI_CALL: {
        msg.data.mpi.other = GET_INT;
        msg.data.mpi.tag = GET_INT;

        msg.data.mpi.other2 = GET_INT;
        msg.data.mpi.tag2 = GET_INT;

        msg.data.mpi.comm = GET_COMM;
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);
    } break;
            
    case BCAST_MPI_CALL: {
    msg.data.mpi.size = GET_INT; // count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype)); // data type
        msg.data.mpi.root = GET_INT;  // root node
        msg.data.mpi.comm = GET_COMM; // communicator
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size); // get communicator size
    } break;
    
    case IBCAST_MPI_CALL: {
    msg.data.mpi.size = GET_INT; // count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype)); // data type
        msg.data.mpi.root = GET_INT; // root node
        msg.data.mpi.comm = GET_COMM; // communicator
        msg.data.mpi.req_data.req_addr = GET_REQADDR; // real memory address of the request
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case ALLTOALLV_MPI_CALL: {
        int* sendCountsStart = (int*)GET_ADDR;// starting address of sendcounts
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        int* recvCountsStart = (int*)GET_ADDR;// starting address of recvcounts
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.opt_recvdatatype));// data type
        msg.data.mpi.comm = GET_COMM;// communicator
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
        
        cleanMem=true;
        msg.data.mpi.many.sizes = new int[msg.data.mpi.comm_size];
        msg.data.mpi.many.opt_recvsizes = new int[msg.data.mpi.comm_size];
        for(int c = 0 ; c < msg.data.mpi.comm_size ; c++){// get counter arrays
            msg.data.mpi.many.sizes[c] = sendCountsStart[c];
            msg.data.mpi.many.opt_recvsizes[c] = recvCountsStart[c];
        }
    } break;
    
    case ALLTOALL_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.opt_recvsize = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.opt_recvdatatype));// data type
        msg.data.mpi.comm = GET_COMM;// communicator
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case IALLTOALL_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.opt_recvsize = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.opt_recvdatatype));// data type
        msg.data.mpi.comm = GET_COMM;// communicator
        msg.data.mpi.req_data.req_addr = GET_REQADDR;// real memory address of the request
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case GATHER_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.opt_recvsize = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.opt_recvdatatype));// data type
        msg.data.mpi.root = GET_INT;// root node
        msg.data.mpi.comm = GET_COMM;// communicator
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case IGATHER_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.opt_recvsize = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.opt_recvdatatype));// data type
        msg.data.mpi.root = GET_INT;// root node
        msg.data.mpi.comm = GET_COMM;// communicator
        msg.data.mpi.req_data.req_addr = GET_REQADDR;// real memory address of the request
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case ALLGATHER_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.opt_recvsize = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.opt_recvdatatype));// data type
        msg.data.mpi.comm = GET_COMM;// communicator
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case IALLGATHER_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.opt_recvsize = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.opt_recvdatatype));// data type
        msg.data.mpi.comm = GET_COMM;// communicator
        msg.data.mpi.req_data.req_addr = GET_REQADDR;// real memory address of the request
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case REDUCE_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.root = GET_INT;// root node
        msg.data.mpi.comm = GET_COMM;// communicator
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case IREDUCE_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.root = GET_INT;// root node
        msg.data.mpi.comm = GET_COMM;// communicator
        msg.data.mpi.req_data.req_addr = GET_REQADDR;// real memory address of the request
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case ALLREDUCE_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.comm = GET_COMM;// communicator
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
    
    case IALLREDUCE_MPI_CALL: {
        msg.data.mpi.size = GET_INT;// count of the data elements
        MPI_Type_size( GET_TYPE ,&(msg.data.mpi.datatype));// data type
        msg.data.mpi.comm = GET_COMM;// communicator
        msg.data.mpi.req_data.req_addr = GET_REQADDR;// real memory address of the request
        MPI_Comm_size(msg.data.mpi.comm, &msg.data.mpi.comm_size);// get communicator size
    } break;
        
    case INIT_MPI_CALL: {
        } break;
    case FIN_MPI_CALL: {
        } break;
    default:
        fprintf(stderr, "PISA-warning: unknown MPI call message\n");
        exit(EXIT_FAILURE);
    }

    va_end(argp);
    process_msg(&msg);

    if(cleanMem){
        if(msg.data.mpi.many.reqs!=NULL)
            delete[] msg.data.mpi.many.reqs; //free(msg.data.mpi.many.reqs);
        if(msg.data.mpi.many.sizes!=NULL)
            delete[] msg.data.mpi.many.sizes;
        if(msg.data.mpi.many.opt_recvsizes!=NULL)
            delete[] msg.data.mpi.many.opt_recvsizes;
        
        msg.data.mpi.many.reqs = NULL;
        msg.data.mpi.many.sizes = NULL;
        msg.data.mpi.many.opt_recvsizes = NULL;
    }
}

