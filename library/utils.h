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

#ifndef __UTILS_ANALYSIS_H
#define __UTILS_ANALYSIS_H


#include <list>
#include <mpi.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "llvm/Support/raw_ostream.h"

#define ANALYZE_ILP             1
#define ANALYZE_ILP_CTRL        2
#define ANALYZE_ILP_VERBOSE     4
#define ANALYZE_ILP_IGNORE_CTRL 8
#define ANALYZE_ITR             16
#define ANALYZE_DTR             32
#define ANALYZE_REG_COUNT       64
#define PRINT_LOAD_STORE        128
#define ANALYZE_MPI_CALLS       256
#define ANALYZE_ILP_TYPE        512
#define PRINT_DEBUG             1024
#define PRINT_BRANCH            2048
#define PRINT_BRANCH_COND       4096
#define ANALYZE_OPENMP_CALLS    8192
#define ANALYZE_MPI_MAP         16384
#define ANALYZE_MPI_DATA        32768
#define ANALYZE_MEM_FOOTPRINT   65536
#define ANALYZE_EXTERNALLIBS_CALLS  131072

#define READ_OPERATION  0
#define WRITE_OPERATION 1

#define BASICBLOCK_NOTIFICATION 1
#define END_APP_NOTIFICATION    2
#define MEM_ADDR_NOTIFICATION   3
#define MPI_UPDATE_PROCESS_ID   4
#define MPI_CALL_NOTIFICATION   5
#define FUNC_ADDRESS            6

// Used by the instruction mix
#define TY_SCALAR   0
#define TY_VECTOR   1
#define TY_MISC     2

#define VECTOR_SIZE_SCALAR  0
#define VECTOR_SIZE_MISC  0

#define PUT_SEND_INFO   1
#define GET_SEND_INFO   2
#define PUT_RECV_INFO   3
#define GET_RECV_INFO   4

#define INFO_NOT_FOUND  6

#define FIRST_SEND_BUFFER   1
#define SECOND_RECV_BUFFER  2

#define INT_4BITS       114
#define INT_8BITS       118
#define INT_16BITS      1116
#define INT_32BITS      1132
#define INT_64BITS      1164
#define INT_MISCBITS    1199

#define PRINT_ONLY_INTS     0
#define PRINT_ONLY_FLOATS   1
#define PRINT_FLOATS_INTS   2

using namespace std;

struct inst_notification_msg {
    int f_id;
    int bb_id;
    int thread_id;
};

struct mem_addr_notification_msg {
    int f_id;
    int bb_id;
    int i_id;
    void *addr;
};

struct mpi_request_data{
    void *req_addr;
    int source;
} ;

struct mpi_call_notification_msg {
    int f_id;
    int bb_id;
    int i_id;
    int other;
    int root; // for collective communications using a root node
    int tag;
    int size;
    int datatype;
    int opt_recvsize;       // for those routines having sentdata and recvdata, this optional parameter stores the recvcount
    int opt_recvdatatype;   // for those routines having sentdata and recvdata, this optional parameter stores the size of recvtype
    MPI_Comm comm;
    int comm_size;
    int other2;
    int tag2;
    int flag;
    struct mpi_request_data req_data;
    struct {
        struct mpi_request_data* reqs; // stores an array of mpi to manage Waitall, Waitsome, Waitany, Testall, Testsome, Testany
        int counter; // stores an integer (the array length of either reqs or (sizes and opt_recvsizes), depending on the MPI call type).
        int* sizes;
        int* opt_recvsizes;
    } many;
};

struct message {
    char type;
    union {
        struct inst_notification_msg bb_notif;
        struct mem_addr_notification_msg mem_notif;
        struct mpi_call_notification_msg mpi;
        int pid;
    } data;
};

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

using namespace llvm;

struct state {
    Function::iterator BB;
    BasicBlock::iterator I;
};

// Returns LLVM function representation for a given address
Function * getFunctionAddress(void *addr);
extern bool weDecoupled;

// Returns memory access size for a load/store instruction
extern unsigned getOperandSize(Instruction *I);

extern vector<map<Instruction *, void *>> MemoryForThreads;

// This method is used to get the real memory addresses.
void * getMemoryAddress(Instruction *I, const int thread_id);

typedef struct {
    char type; // this field has the following meaning:
            // 1 - the client is sending MPI_?send info
            // 2 - the client wants to extract info for MPI_?send
            // 3 - the client is sending MPI_?recv info
            // 4 - the client wants to extract info for MPI_?recv
            // Tip: notice that (type & 1) is 0 if the server has
            // to send back information
            // 6 - only in messages from the server to the client;
            //     it means the query was not found (yet)
    int id_s;
    int id_r;
    int tag;
    MPI_Comm comm;
    int mpi_status_source;
    unsigned long long issue_cycle;
    unsigned long long inst;
    unsigned long long dif;
    Instruction *I;
    unsigned long long msgSize;
} __attribute__((packed)) MPIcall;

struct MPIinfo {
    MPIcall info;
    unsigned long long index;
    unsigned long long dif;
    int count;
    MPI_Datatype dtype;
    void *real_addr;
};

struct MPIcall_node {
    MPIcall info;
    struct MPIcall_node *next;
};

typedef struct {
    std::unique_ptr<list<MPIcall>> s_call;
    std::unique_ptr<list<MPIcall>> r_call;
    // Add lists for other MPI communication functions
    // eg: list<MPIcall> *allreduce_call;
    // struct MPIcall_node *s_call_coupledImplemenation;
    // struct MPIcall_node *r_call_coupledImplemenation;
} MPIdb;

// Instantiated in utils.cc
extern int max_expected_threads;  

Module::iterator get_function(int f, Module *M);
Function::iterator get_basicblock(int f, int bb, Module *M);
BasicBlock::iterator get_instruction(int f, int bb, int i, Module *M);
Function * get_calledFunction(CallInst *call);
bool mpiMatchName(string,const char*);
unsigned long long upperPowerOfTwo(unsigned long long v);

// Map between real memory address and LLVM's Function *
// m_lock is used to sync access to FunctionsAddresses
extern pthread_mutex_t m_lock;

extern map<void *, Function *> FunctionsAddresses;
extern vector<bool> InitializedThreads;
extern vector<map<Instruction *, void *>> MemoryForThreads;

// Maps for saving states when a internal call is processed
extern vector<vector<struct state>> SavedStatesForThreads;

// Round double number with 4 decimals
double double4(double x);

// Comparator used by the set_intersection function
bool map_less(std::pair<const unsigned long long, bool> &v1, 
              std::pair<const unsigned long long, bool> &v2);
bool map_less_(std::pair<const unsigned long long, unsigned long long> &v1, 
               std::pair<const unsigned long long, unsigned long long> &v2);


// Function used to find the shared addresses between two sets of sorted keys
void intersectTwoMaps(map<unsigned long long, bool> &m1, 
                      map<unsigned long long, bool> &m2,
                      std::vector<std::pair<unsigned long long, bool> > &output);

void intersectTwoMapsLong(map<unsigned long long, unsigned long long> &m1, 
                          map<unsigned long long, unsigned long long> &m2,
                          std::vector<std::pair<unsigned long long, unsigned long long> > &output);

// Function used to find the shared addresses when there are more than two sets of sorted keys
void intersectMore(std::vector<std::pair<unsigned long long, bool> >& crtOutputOrig, 
                   map<unsigned long long, bool> &m,
                   std::vector<std::pair<unsigned long long, bool> > &output);

void intersectMoreLong(std::vector<std::pair<unsigned long long, unsigned long long> >& crtOutputOrig, 
                       map<unsigned long long, unsigned long long> &m,
                       std::vector<std::pair<unsigned long long, unsigned long long> > &output);
                   
int is_mpi_sync_call(Function *_call);

#endif
