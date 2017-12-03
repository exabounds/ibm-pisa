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

#include <mpi.h>
#include <omp.h>
#include <assert.h>

#include <signal.h>
#include <llvm/Support/raw_os_ostream.h>

#include "utils.h"
#include "safe_queue.h"
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

#include <map>

// Server's IP:PORT
char *ip = NULL;
int portno = -1;

struct thread_specific_data {
    int sockfd;
};

vector<struct thread_specific_data> DataForThreads;

int safeWrite(int fd, char* buffer, int count) {
    int c = write(fd, buffer, count);
    if (c <= 0) return c;
    int totalCount = c;
    while (totalCount < count) {
        c = write(fd, buffer+totalCount, count-totalCount);
        assert(c>0);
        totalCount += c;
    }
    assert(totalCount == count);
    return totalCount;
}


static int connect_to_server() {
    int sockfd = -1;

    // Open socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: can not open socket!\n");
        exit(EXIT_FAILURE);
    }

    // Add info for SERVER_IP:PORT
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Error: can not connect to %s:%d\n", ip, portno);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// If it is the first time this thread enters the library, it will open
// a connection with the server and save the information in the thread_specific_data structure.
// If the connection is already opened, the function will just return the information.
static void get_thread_specific_data() {
    // FIXME: in OpenMP, only the master thread pass through the init_libanalysis() and get_thread_specific_data().
    // FIXME: thus the commented code would not work in either cases.
    // FIXME: the uncommented code works at least for MPI (MPI_Init called within the init_libanalysis).
    
    /*
    const int thread_id = omp_get_thread_num();
    if (InitializedThreads[thread_id]) {
        return;
    }

    InitializedThreads[thread_id] = true;

    int sockfd = connect_to_server();

    struct thread_specific_data thread_data;
    memset(&thread_data, 0, sizeof(thread_data));
    thread_data.sockfd = sockfd;

    DataForThreads[thread_id] = thread_data;
    */
    
    int procId;
    MPI_Comm_rank(MPI_COMM_WORLD, &procId);
    
    // FIXME: this should be always 0. OpenMP apps in any case pass through here only for the master thread.
    // FIXME: Additionally, max_expected_threads in the client is set to 1!
    const int thread_id = omp_get_thread_num(); 
    
    if (InitializedThreads[thread_id])
        return;

    InitializedThreads[thread_id] = true;
    int sockfd = connect_to_server();
    struct thread_specific_data thread_data;
    memset(&thread_data, 0, sizeof(thread_data));
    thread_data.sockfd = sockfd;
    DataForThreads[thread_id] = thread_data;
}

static void send_msg(struct message *msg) {
    static unsigned long long total_msg_sent = 0;

    // get_thread_specific_data(); 
    // This would be required for OpenMP applications (bugs araise with MPI due to MPI_Comm_rank after MPI_Finalize). 
    // However bugs should exist also for OpenMP because the thread_id from OpenMP missmatches the thread_id from accept_new_connection.

    int n = safeWrite(DataForThreads[omp_get_thread_num()].sockfd, (char*) msg, sizeof(struct message));
    if (n < 0) {
        fprintf(stderr, "Error: writing to socket\n");
        exit(EXIT_FAILURE);
    }

    total_msg_sent++;

    if (n != sizeof(struct message))
        fprintf(stderr,"ERROR CLIENT %d %lu\n", n, sizeof(struct message));

    // DEBUG
    /*
    switch(msg->type) {
        case BASICBLOCK_NOTIFICATION: {
            if (msg->data.bb_notif.f_id < 0 || msg->data.bb_notif.f_id > 200)
            fprintf(stdout,"add %p t_id %d f_id %d bb_id %d\n", msg, msg->data.bb_notif.thread_id, msg->data.bb_notif.f_id, msg->data.bb_notif.bb_id);
            break;
        }        
    }
    */

}

// This function initialize the libanalysis. Calls different constructors
// and parse the LLVM IR string.
extern "C" void init_libanalysis(char *params,int argc, char ** argv) {
    MPI_Init(&argc,&argv);

    char *params_copy = strdup(params);
    char *p = strtok(params_copy, ":/");
    ip = p;
    p = strtok(NULL, ":/");
    portno = atoi(p);
    p = strtok(NULL, ":/");
    max_expected_threads = atoi(p); // FIXME: max_expected_thread is set on the server but this runs on the client. Is it a bug for OpenMP decoupled?

    DataForThreads = vector<struct thread_specific_data>(max_expected_threads);
    InitializedThreads = vector<bool>(max_expected_threads, false);

    get_thread_specific_data();
}

// This hook extracts information about the current executing basic block
extern "C" void inst_notification(int f, int bb) {
    struct message msg;

    memset(&msg, 0, sizeof(msg));

    msg.type = BASICBLOCK_NOTIFICATION;
    msg.data.bb_notif.f_id = f;
    msg.data.bb_notif.bb_id = bb;
    msg.data.bb_notif.thread_id = omp_get_thread_num();

    send_msg(&msg);
}

// This function receives the real memory address of a given
// load/store instruction. It updates the mapping and
// then it continues the processing of the current basic block,
// starting from the load/store instruction.
extern "C" void update_vars(int f, int bb, int i, ...) {
    va_list argp;

    va_start(argp, i);
    void *value = va_arg(argp, void *);
    va_end(argp);

    struct message msg;

    memset(&msg, 0, sizeof(msg));

    msg.type = MEM_ADDR_NOTIFICATION;
    msg.data.mem_notif.f_id = f;
    msg.data.mem_notif.bb_id = bb;
    msg.data.mem_notif.i_id = i;

    msg.data.mem_notif.addr = value;

    send_msg(&msg);
}

extern "C" void func_address(int f, ...) {
    va_list argp;
    va_start(argp, f);
    void *value = va_arg(argp, void *);
    va_end(argp);

    struct message msg;
    memset(&msg, 0, sizeof(msg));

    msg.type = FUNC_ADDRESS;
    msg.data.mem_notif.f_id = f;
    msg.data.mem_notif.addr = value;

    send_msg(&msg);
}

extern "C" void mpi_update_process_id(void) {
    MPI_Barrier(MPI_COMM_WORLD); // Makes sure that all processes established connection to the server (i.e. data-structure allocated on the server side)
    struct message msg;

    memset(&msg, 0, sizeof(msg));
    msg.type = MPI_UPDATE_PROCESS_ID;

    MPI_Comm_rank(MPI_COMM_WORLD, &msg.data.pid);

    send_msg(&msg);
    MPI_Barrier(MPI_COMM_WORLD); // Not sure if this is needed but let's be sure that all processes communicated their ids before communicating anything
}


void process_msg(struct message *msg){
    send_msg(msg);
}

extern "C" void end_app(void) {
    struct message msg;

    memset(&msg, 0, sizeof(msg));
    msg.type = END_APP_NOTIFICATION;
    send_msg(&msg);

    MPI_Finalize();
}
