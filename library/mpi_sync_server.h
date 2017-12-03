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

#ifndef MPI_SYNC_SERVER__H
#define MPI_SYNC_SERVER__H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <mpi.h>

struct MPIupdate {
    void *call;
    void *s_buf;
    void *s_value;
    int *s_vcount;
    int s_count;
    int *s_depl;
    MPI_Datatype s_dtype;
    void *r_buf;
    void *r_value;
    int *r_vcount;
    int r_count;
    int *r_depl;
    MPI_Datatype r_dtype;
    unsigned long long index;
    unsigned long long dif;
};

#endif // MPI_SYNC_SERVER__H
