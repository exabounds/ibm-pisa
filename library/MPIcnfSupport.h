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

#ifndef LLVM_MPIcnfSupport_H_
#define LLVM_MPIcnfSupport_H_

#include "utils.h"
int is_mpi_sync_call(Function *_call);

#define BASIC_MPI_CALL  1
#define ASYNC_MPI_CALL  2
#define ANYTAG_MPI_CALL   3
#define SENDRECV_MPI_CALL 4
#define WAIT_MPI_CALL   5
#define TEST_MPI_CALL   6
#define ALL_MPI_CALL    7
#define INIT_MPI_CALL   8
#define FIN_MPI_CALL    9
#define WAITALL_MPI_CALL   10
#define TESTALL_MPI_CALL   11
#define BCAST_MPI_CALL    12
#define IBCAST_MPI_CALL   13
#define ALLTOALL_MPI_CALL   14
#define IALLTOALL_MPI_CALL  15
#define GATHER_MPI_CALL    16
#define IGATHER_MPI_CALL   17
#define ALLGATHER_MPI_CALL  18
#define IALLGATHER_MPI_CALL 19
#define REDUCE_MPI_CALL    20
#define IREDUCE_MPI_CALL   21
#define ALLREDUCE_MPI_CALL  22
#define IALLREDUCE_MPI_CALL 23
#define ALLTOALLV_MPI_CALL  24

#define GET_INT_FROM_FORTRAN (*( va_arg(argp, int*)))                       /* i32* type in LLVM IR... Pointer due to fortran parameter passing */
#define GET_COMM_FROM_FORTRAN (MPI_Comm_f2c( *(va_arg(argp, MPI_Fint*)) ))  /* i32* type in LLVM IR ... should be taken from MPI_Fint <- int */
#define GET_REQADDR_FROM_FORTRAN ( va_arg(argp, void*))  // NOTE, we do not need to convert f2c the request, we need only the request address identifier!
#define GET_ADDR_FROM_FORTRAN ( va_arg(argp, void*))    // NOTE, we do not need to convert f2c the request, we need only the request address identifier!
#define GET_TYPE_FROM_FORTRAN (MPI_Type_f2c( *(va_arg(argp, MPI_Fint*)) ))  /* i32* type in LLVM IR ... should be taken from MPI_Fint <- int */
//#define GET_REQ_FROM_FORTRAN (MPI_Request_f2c( *va_arg(argp, MPI_Fint*)))  // This would convert the request content but loose its address identifier.

#define GET_INT          ( isFortran ? GET_INT_FROM_FORTRAN    : va_arg(argp, int) )
#define GET_INT_FROM_POINTER   ( isFortran ? GET_INT_FROM_FORTRAN    : (*( va_arg(argp, int*)))  )
#define GET_COMM         ( isFortran ? GET_COMM_FROM_FORTRAN   : va_arg(argp, MPI_Comm) )
#define GET_REQADDR        ( isFortran ? GET_REQADDR_FROM_FORTRAN  : va_arg(argp, void *) )
#define GET_ADDR         ( isFortran ? GET_ADDR_FROM_FORTRAN    : va_arg(argp, void *) )
#define GET_TYPE         ( isFortran ? GET_TYPE_FROM_FORTRAN   : va_arg(argp, MPI_Datatype) )

#define CONVERT_STATUS_POINTER(src,dst) \
  MPI_Status statusInMemory; \
  if(isFortran){ \
   MPI_Status_f2c((MPI_Fint*) src, &statusInMemory); \
   dst = &statusInMemory; \
  } \
  else \
   dst = (MPI_Status *) src;
  
extern "C" void mpi_update_db(int f, int bb, int i, int isFortran, ...);

// This is a library-specific implementation defined in libanalysisXXX.cc
void process_msg(struct message *msg);

#endif
