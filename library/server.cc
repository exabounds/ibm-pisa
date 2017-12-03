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

#include <getopt.h>
#include <pthread.h>
#include <signal.h>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

#include <atomic>
#include <memory>
#include <map>
#include <list>

#include "utils.h"
#include "JSONmanager.h"
#include "safe_queue.h"
#include <string>

#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm-c/Support.h"
#include "llvm/Support/Threading.h"


#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IRReader/IRReader.h"
#if LLVM_VERSION_MINOR > 4
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/InstIterator.h"
#else
#include "llvm/Assembly/Parser.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/system_error.h"
#endif

#include "WKLDchar.h"
#include "MPIcnfSupport.h"

using namespace std;
using namespace llvm;

#define MAXEVENTS   102400

string moduleFilename;
string moduleExe;
char *branch_entropy_file = "";

char *ip = NULL;
int portno = -1;
bool end_all_threads_once = false;
static pthread_mutex_t should_stop_lock = PTHREAD_MUTEX_INITIALIZER;
vector<bool> should_stop;

unsigned long long options = 0;
int data_cache_line_size = 0;
int data_reuse_distance_resolution = 0;
int data_reuse_distance_resolution_final_bin = 0;
int inst_cache_line_size = 0;
int inst_size = 0;
int ilp_type = 0;
int debug_flag = 0;
int window_size = 0;
int accMode = 0;

char *AppName  = "default-app";
char *TestName = "default-test";
char *PISAFileName = "";
char *AddJSONData = "";

vector<WKLDchar> WKLDcharForThreads;

vector<string> ExcludedFunctions;
vector<string> IncludeFunctions;

std::unique_ptr<MPIdb> mpi_map_db;
static pthread_mutex_t mpi_db_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ilp_dbg_output_lock = PTHREAD_MUTEX_INITIALIZER;

struct connection_data {
    int sockfd;
    pthread_t thread;
    std::unique_ptr<safe_queue> jobs;
    int mpi_processor_id;
    char a;
    std::unique_ptr<rapidjson::PrettyWriter<rapidjson::StringBuffer>> JSONwriter;
    std::unique_ptr<rapidjson::StringBuffer> JSONbuffer;
    int inFunction;
    int thread_id;
};

// This is a map between a socket-id and its corresponding private data.
// Each socket corresponds to a thread from the running binary.
map<int, struct connection_data *> threads_data;

// pthread_id of the master thread
pthread_t master_thread;

static int thread_counter = 0;
std::atomic<int> total_num(0);
std::atomic<int> toDump(0);
int sockfd;
struct epoll_event *events = NULL;

/*
    TODO: DEBUG!
    This can generate leakage in theory but the module M is created once and then only always read.
    There seems to be a problem with multiple threads when they are exiting they tend to free the unique pointer multiple time.
    This problem might be related to something different than multi-threaded but still, it might generate segmentation fault on exit.
    I prefer not to free this memory rather than segment because we need to load once at the very beginning and free only on exit.

    std::unique_ptr<Module> M;
*/

struct noDestroy_Functor {
    void operator() (Module *m) const {
    // if (total_num == 0 && m != NULL)
    //  delete m;
       return;
    }
};

std::unique_ptr<Module,noDestroy_Functor> M;

// This lock is used to sync the access for dumping information.
// For example, during the LoadStoreVerbose analysis, the
// information is printed dynamically and it is not collected
// in the memory. All threads performs I/O operations
// so the printing process must be mutual exclusive done.
static pthread_mutex_t ls_lock = PTHREAD_MUTEX_INITIALIZER;


// reading waiting errors on the socket
// return 0 if there's none, 1 otherwise
int socket_check(int fd) {
   int ret;
   int code;
   socklen_t len = sizeof(int);

   ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &code, &len);

   if ((ret || code)!= 0)
      return 1;

   return 0;
}

// read from socket descriptor fd
int safeRead(int fd, char* buffer, int count) {
    int c = read(fd, buffer, count);
    
    if (c <= 0) return c;
    int totalCount = c;
    
    while (totalCount < count) {
        c = read(fd, buffer+totalCount, count-totalCount);
        assert(c>0);
        totalCount += c;
    }
    
    assert(totalCount == count);
    return totalCount;
}

static void print_mpi_db() {
    list<MPIcall>::iterator it;

    pthread_mutex_lock(&mpi_db_lock);

    fprintf(stderr, "SEND msg:\n");
    it = mpi_map_db->s_call->begin();
    for (; it != mpi_map_db->s_call->end(); it++) {
        MPIcall iter = *it;
        fprintf(stderr, "type=%d id_s=%d id_r=%d index=%llu\n", iter.type, iter.id_s, iter.id_r, iter.inst);
    }

    fprintf(stderr, "\nRECV msg:\n");
    it = mpi_map_db->r_call->begin();
    for (; it != mpi_map_db->r_call->end(); it++) {
        MPIcall iter = *it;
        fprintf(stderr, "type=%d id_s=%d id_r=%d index=%llu\n", iter.type, iter.id_s, iter.id_r, iter.inst);
    }

    pthread_mutex_unlock(&mpi_db_lock);
}

static int boot_server() {
    int sockfd = -1;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: cannot open socket\n");
        exit(EXIT_FAILURE);
    }

    // Add info for SERVER_IP:PORT
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(portno);

    // Bind socket
    int ret = -1;
    ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        fprintf(stderr, "Error: cannot bind socket\n");
        exit(EXIT_FAILURE);
    }

    // Change socket to listening mode
    listen(sockfd, MAXEVENTS + 1);

    return sockfd;
}

// This function verifies is a basic block is inside a given function
static bool check_if_bb_is_in_function(Function *F, Function::iterator BB_i) {
    // errs() << "F->getName().str() " << F->getName().str() << "\n";

    assert(!(F->getName().str().empty()));
    assert(!(BB_i->getParent()->getName().str().empty()));

    assert(F->getName().str().compare("") != 0);
    assert(BB_i->getParent()->getName().str().compare("") != 0);

    if (BB_i->getParent()->getName().str().compare(F->getName()) == 0)
            return true;

    return false;
}

// saved_states is a vector that memories the call trace.
// This function verifies if the current basic block
// is inside an included function for the analysis
// or the SavedStates contains such a function.
static bool check_if_bb_is_in_included_functions(Function::iterator BB, const int thread_id) {
    if (!M)
        return false;

    if (IncludeFunctions.size() == 0)
        return true;

    for (unsigned i = 0; i < IncludeFunctions.size(); i++) {
        Function *F = M->getFunction(IncludeFunctions[i]);
        if (F && check_if_bb_is_in_function(F, BB)) {
            return true;
        }
    }

    for (unsigned i = 0; i < IncludeFunctions.size(); i++) {
        Function *F = M->getFunction(IncludeFunctions[i]);
        for (unsigned j = 0; j < SavedStatesForThreads[thread_id].size(); j++) {
            struct state previous = SavedStatesForThreads[thread_id].at(j);
            if (F && check_if_bb_is_in_function(F, previous.BB)) {
                return true;
            }
        }
    }

    return false;
}

// saved_states is a vector that memories the call trace.
// This function verifies if the current basic block
// is inside an excluded function for the analysis
// of the SavedStates contains such a function.
static bool check_if_bb_is_in_excluded_functions(Function::iterator BB, const int thread_id) {
    if (!M || ExcludedFunctions.size() == 0)
        return false;

    for (unsigned i = 0; i < ExcludedFunctions.size(); i++) {
        Function *F = M->getFunction(ExcludedFunctions[i]);
        if (F && check_if_bb_is_in_function(F, BB)) {
            return true;
        }
    }

    if (SavedStatesForThreads[thread_id].size() == 0)
        return false;

    for (unsigned i = 0; i < ExcludedFunctions.size(); i++) {
        Function *F = M->getFunction(ExcludedFunctions[i]);
        for (unsigned j = 0; j < SavedStatesForThreads[thread_id].size(); j++) {
            struct state previous = SavedStatesForThreads[thread_id].at(j);
            if (F && check_if_bb_is_in_function(F, previous.BB)) {
                return true;
            }
        }
    }

    return false;
}

// This function is called if a return instruction is encounter inside the
// 'main' function. It dumps the data collected by the framework.
void dump_analysis() {
    JSONmanager JSONman(0, options, &WKLDcharForThreads);

    if(strlen(PISAFileName)==0)
        PISAFileName = getenv("PISAFileName");
    
    JSONman.openMaster(PISAFileName, AddJSONData, AppName, TestName);
    JSONman.dump(0);

    JSONman.close();
}

// This function process an instruction
static void process_instr(connection_data *data, Instruction *I) {
    // 'analyze' function is responsible for calling the
    // 'visit' function for each active analysis
    WKLDcharForThreads[data->thread_id].analyze(*I);

    // If the current instruction is a call to a function
    // named 'exit', the application will end so we need
    // to dump all the extracted information by manually
    // calling 'dump_analysis' function.
    if (strcmp(I->getOpcodeName(), "phi")) {
        if (!strcmp(I->getOpcodeName(), "call")) {
            CallInst *call = cast<CallInst>(I);
            Function *F = get_calledFunction(call);

            if (F && F->getName().str() == "exit") {
                //dump_analysis(data->thread_id, data->JSONwriter.get(), data->JSONbuffer.get());
                pthread_exit(NULL);
            }
        }
    }
}

// This function is responsible of processing a basic block, starting
// from the I_init instruction. The procesing is done until the end
// of the basic block or until is interrupted because it found
// a call to an internal function, a load/store instruction or
// a MPI call.
static void iter_instructions(const int thread_id, connection_data* data,
                              Function::iterator BB, BasicBlock::iterator I_init) {
    Instruction *last_processed_inst = NULL;

    for (BasicBlock::iterator I = I_init, J = BB->end(); I != J; ++I) {
        Function *_call = NULL;
        if (!strcmp(I->getOpcodeName(), "call")) {
            CallInst *call = cast<CallInst>(I);
            _call = get_calledFunction(call);

            // We are ignoring LLVM's debugging functions
            if (_call && _call->getName().str().compare(0,5,"llvm.")==0)
                continue;
        }

        // If the current instruction is a load/store instruction, the
        // processing will be interrupt and will be restored by the
        // 'update_var' call, after the real memory address is updated.
        if (!strcmp(I->getOpcodeName(), "load") ||
                !strcmp(I->getOpcodeName(), "store"))
            return;

        // The instruction is processed only if it's contained in a
        // included function and it is not contained in a excluded function.
        if (check_if_bb_is_in_included_functions(BB, thread_id))
            if (!check_if_bb_is_in_excluded_functions(BB, thread_id))
                process_instr(data, I);

        // We memorized the last processed instruction
        last_processed_inst = I;

        // If the current instruction is a 'call' instruction, we need
        // to verify the type of the 'call' instruction. If the 'call'
        // is to an internal function, the processing will be interrupt
        // and will be restored by the next 'inst_notification' call.
        if (!strcmp(I->getOpcodeName(), "call")) {
            if (_call && _call->begin() != _call->end()) {
                // call to a internal module function; push state
                struct state current_state;
                current_state.BB = BB;
                ++I;
                current_state.I = I;
                
                if (IncludeFunctions.size() > 0 && data->inFunction >= 1) 
                    data->inFunction++;
                
                // current_state.inFunction = data->inFunction;
                SavedStatesForThreads[thread_id].push_back(current_state);
                return;
            }
            if (options & ANALYZE_MPI_MAP) {
                if (_call && is_mpi_sync_call(_call))
                    return;
            }
        }
    }

    // If we ended up here it means that the processing ended because it
    // reached the end of the basic block. We look if we have any saved
    // state. If we have such a state, it means that we were previously
    // interrupted by a call to an internal function and we must restore
    // the processing from the instruction after the call.
    if (!SavedStatesForThreads[thread_id].empty() && last_processed_inst && !strcmp(last_processed_inst->getOpcodeName(), "ret")) {
        struct state next_state = SavedStatesForThreads[thread_id].back();
        SavedStatesForThreads[thread_id].pop_back();

        // The 'call' instruction is a special instruction for ILP.
        // We should update the IssueCycle of the instruction
        // (ergo the IssueCycle of the variable that gets the result
        // of the call) to be the same as it was for the return
        // instruction.
        next_state.I--;
        WKLDcharForThreads[data->thread_id].updateILPforCall(last_processed_inst, next_state.I);
        next_state.I++;

        if (IncludeFunctions.size() > 0 && data->inFunction > 1) data->inFunction--;

        if (data->inFunction == 1)
        // if (next_state.inFunction == 1)
            if (!check_if_bb_is_in_included_functions(next_state.BB, thread_id)) {
                data->inFunction = 0;
                WKLDcharForThreads[data->thread_id].process_mpi_map(NULL, NULL, 0, data->inFunction);
            }

        iter_instructions(thread_id, data, next_state.BB, next_state.I);
    }
}

static void update_memory(Instruction *I, void *addr, const int thread_id) {
    MemoryForThreads[thread_id][I] = addr;
}

static void *queue_consumer(void *ptr) {
    struct connection_data *data = (struct connection_data *)ptr;
    data->inFunction = 0;
    safe_queue *task_list = data->jobs.get();

    // TODO!
    // if (IncludeFunctions.size() == 0)
    // data->inFunction = 2;

    // This function is responsible for reconstructing the thread flow.
    while (!should_stop[data->thread_id]) {
        struct message msg = task_list->pop_front();

        switch(msg.type) {
            case BASICBLOCK_NOTIFICATION: {
                Function::iterator BB = get_basicblock(msg.data.bb_notif.f_id,
                                   msg.data.bb_notif.bb_id, M.get());

                if (!InitializedThreads[data->thread_id]) {
                    int thread_id = msg.data.bb_notif.thread_id;

                    InitializedThreads[data->thread_id] = true;
                    
                    WKLDcharForThreads[data->thread_id] = WKLDchar(M.get(), 
                                                                   options, 
                                                                   data_cache_line_size, 
                                                                   data_reuse_distance_resolution,
                                                                   data_reuse_distance_resolution_final_bin, 
                                                                   inst_cache_line_size,
                                                                   inst_size, 
                                                                   ilp_type, 
                                                                   debug_flag, 
                                                                   window_size,
                                                                   thread_id, 
                                                                   0, 
                                                                   &ls_lock, 
                                                                   mpi_map_db.get(), 
                                                                   &mpi_db_lock, 
                                                                   accMode, 
                                                                   &ilp_dbg_output_lock, 
                                                                   branch_entropy_file);
                                                                   
                    MemoryForThreads[data->thread_id] = map<Instruction *, void *>();
                    SavedStatesForThreads[data->thread_id] = vector<struct state>();
                }

                // This is used mainly in the case of a per-function analysis to extract a DIMEMAS trace
                if (options & ANALYZE_MPI_MAP) {
                    if (data->inFunction == 0) {
                        if (check_if_bb_is_in_included_functions(BB, data->thread_id))
                            if (!check_if_bb_is_in_excluded_functions(BB, data->thread_id)) {
                                data->inFunction = 1;
                                WKLDcharForThreads[data->thread_id].process_mpi_map(NULL, NULL, 0, data->inFunction);
                            }
                        } else {
                            // To investigate when this happens!
                            if (!check_if_bb_is_in_included_functions(BB, data->thread_id)) {
                                data->inFunction = 0;
                                WKLDcharForThreads[data->thread_id].process_mpi_map(NULL, NULL, 0, data->inFunction);
                            }
                        }
                }
                iter_instructions(data->thread_id, data, BB, BB->begin());
                break;
            }
            case MEM_ADDR_NOTIFICATION: {
                Function::iterator BB = get_basicblock(msg.data.mem_notif.f_id, msg.data.mem_notif.bb_id, M.get());
                BasicBlock::iterator I = get_instruction(msg.data.mem_notif.f_id,
                                                         msg.data.mem_notif.bb_id,
                                                         msg.data.mem_notif.i_id, 
                                                         M.get());

                update_memory(I, msg.data.mem_notif.addr, data->thread_id);

                // if bb is in included functions, but not in exclude
                if (check_if_bb_is_in_included_functions(BB, data->thread_id))
                    if (!check_if_bb_is_in_excluded_functions(BB, data->thread_id))
                        process_instr(data, I);

                I++;

                if (I != BB->end())
                    iter_instructions(data->thread_id, data, BB, I);
                break;
            }

            case MPI_UPDATE_PROCESS_ID: {       // This should be the very first call for MPI application and should make sure to update data->threadid
                data->mpi_processor_id = msg.data.pid;
                data->thread_id = msg.data.pid; // FIXME: all data structures use only thread_id as identifier. Thus we do not support OpenMP+MPI.
                                                // FIXME: also OpenMP is not supported:
                                                // we may need to fix the max_number_of_threads also on the client side
                                                // we run a queue_consumer for each connected process, threads are running on the same connection (data)
                                                                                    
                if (!InitializedThreads[data->thread_id]) {
                    int thread_id = msg.data.bb_notif.thread_id;

                    InitializedThreads[data->thread_id] = true;

                    WKLDcharForThreads[data->thread_id] = WKLDchar(M.get(), 
                                                                   options, 
                                                                   data_cache_line_size, 
                                                                   data_reuse_distance_resolution,
                                                                   data_reuse_distance_resolution_final_bin, 
                                                                   inst_cache_line_size,
                                                                   inst_size, 
                                                                   ilp_type, 
                                                                   debug_flag, 
                                                                   window_size,
                                                                   msg.data.pid // FIXME, this had to be thread_id. However this is used as identifier to access data strudtures. 
                                                                                // At the server side, this data-structures are shared among processes, thus we need an unique process identifier, 
                                                                                // not the OpenMP thread_id. Thus: a) we do not support OpenMP and 
                                                                                // b) any analysis on thread-level data sharing is meaningless.
                                                                   , msg.data.pid, 
                                                                   &ls_lock, 
                                                                   mpi_map_db.get(), 
                                                                   &mpi_db_lock, 
                                                                   accMode, 
                                                                   &ilp_dbg_output_lock, 
                                                                   branch_entropy_file);
                                                                   
                    MemoryForThreads[data->thread_id] = map<Instruction *, void *>();
                    SavedStatesForThreads[data->thread_id] = vector<struct state>();
                }
                WKLDcharForThreads[data->thread_id].update_processor_id(msg.data.pid);
                break;
            }
            case MPI_CALL_NOTIFICATION: {
                Function::iterator BB = get_basicblock(msg.data.mpi.f_id, msg.data.mpi.bb_id, M.get());
                BasicBlock::iterator I = get_instruction(msg.data.mpi.f_id,
                                                         msg.data.mpi.bb_id,
                                                         msg.data.mpi.i_id, 
                                                         M.get());

                if (options & ANALYZE_MPI_MAP) {
                    WKLDcharForThreads[data->thread_id].process_mpi_map(I, &msg, 1, data->inFunction);
                }

                if (options & ANALYZE_MPI_DATA) {
                    WKLDcharForThreads[data->thread_id].process_mpi_data(I, &msg);
                }

                I++;

                if (I != BB->end())
                    iter_instructions(data->thread_id, data, BB, I);

                break;
            }
            case FUNC_ADDRESS: {
                // This should happen only in the beginning before any BASICBLOCK message is received
                pthread_mutex_lock(&m_lock);
                Module::iterator F = get_function(msg.data.mem_notif.f_id, M.get());
                FunctionsAddresses[msg.data.mem_notif.addr] = F;
                pthread_mutex_unlock(&m_lock);
                break;
        }
            case END_APP_NOTIFICATION: {
                pthread_mutex_lock(&should_stop_lock);
                should_stop[data->thread_id]=true;
                pthread_mutex_unlock(&should_stop_lock);
                break;
            }
        }

    }

    while(toDump != data->thread_id)
        sched_yield();
        
    toDump++;
    if(toDump == thread_counter)
        dump_analysis();
    // pthread_exit(NULL);

    // while(total_num != 0)
        // sched_yield();
        
    close(sockfd);
    exit(EXIT_SUCCESS);

    return NULL;
}

static void accept_new_connections(int sockfd, int epollfd) {
    struct sockaddr in_addr;
    socklen_t in_len;
    int infd = -1;

    in_len = sizeof(in_addr);
    infd = accept(sockfd, &in_addr, &in_len);
    if (infd < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            // We have processed all incoming connections.
            return;
        else {
            fprintf(stderr, "Error: accept\n");
            return;
        }
    }

    // Add socket to the list of fds to monitor
    struct epoll_event event;
    event.data.fd = infd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event);

    // Add the queue to the list of threads_data
    struct connection_data *data = new connection_data();
    data->sockfd = infd;
    data->jobs.reset(new safe_queue());

    auto JSONbuffer = new rapidjson::StringBuffer(0,100000);
    data->JSONbuffer.reset(JSONbuffer);
    data->JSONwriter.reset(new rapidjson::PrettyWriter<rapidjson::StringBuffer>(*JSONbuffer));

    pair<int, struct connection_data *> element(infd, data);

    threads_data.insert(element);

    // Start the worker
    pthread_t thread;
    data->thread = thread;

    data->thread_id = thread_counter;

    thread_counter++;
    total_num++;
    pthread_mutex_lock(&should_stop_lock);
    should_stop.push_back(false);
    pthread_mutex_unlock(&should_stop_lock);
    pthread_create(&thread, NULL, queue_consumer, (void *)data);
}

static void recv_msg(int fd) {
    int count = 1;
    struct message msg;

    count = safeRead(fd, (char*) &msg, sizeof(msg));
    if (count == 0) {
        // Connection closed
        // Closing the descriptor will also make epoll
        // remove it from the set of descriptors which
        // are monitored
        close(fd);
        return;
    }

    map<int, struct connection_data *>::iterator it;

    it = threads_data.find(fd);
    it->second->jobs->push_back(msg);
}

static void parse_ir_file(const string exe, const string filename) {
    SMDiagnostic Err;
    LLVMContext &Context = getGlobalContext();

#if LLVM_VERSION_MINOR >= 7
    M = parseIRFile(filename.c_str(), Err, Context);
#else
    M.reset(ParseIRFile(filename.c_str(), Err, Context));
#endif
    if (!(M.get())) {
        Err.print(exe.c_str(), errs());
        exit(EXIT_FAILURE);
    }
}

static void print_usage(char *exe_path) {
    fprintf(stderr, "Usage: %s -filename FILENAME ", exe_path);
    fprintf(stderr, "-ip SERVER_IP -portno SERVER_PORTNO [OPTIONS]\n");

    fprintf(stderr, "\t-filename FILENAME - specifies the path to *.bc file\n");
    fprintf(stderr, "\t-ip SERVER_IP - specifies the server's IP address\n");
    fprintf(stderr, "\t-portno SERVER_PORTNO - specifies the server's port number\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-app-name - application name\n");
    fprintf(stderr, "\t-test-name - scenario name\n");
    fprintf(stderr, "\t-msg-dbg - print debug messages\n");
    fprintf(stderr, "\t-exclude - exclude functions from analysis; separated by comma, without space\n");
    fprintf(stderr, "\t-include - include functions from analysis; separated by comma, without space\n");
    fprintf(stderr, "\t-analyze-ilp - activates ILP analysis\n");
    fprintf(stderr, "\t\t-ilp-type - specify the ILP type\n");
    fprintf(stderr, "\t\t-ilp-ctrl - with incremental control instruction\n");
    fprintf(stderr, "\t\t-ilp-ignore-ctrl - ignore all control instructions\n");
    fprintf(stderr, "\t\t-ilp-verbose - verbose output\n");
    fprintf(stderr, "\t\t-window-size - set window size of ILP scheduler\n");
    fprintf(stderr, "\t-analyze-data-temporal-reuse - activates DTR analysis\n");
    fprintf(stderr, "\t-analyze-memory-footprint - activates memory footprint analysis when DTR analysis is enabled\n");
    fprintf(stderr, "\t\t-data-cache-line-size - 0 is equivalent of not using this option\n");
    fprintf(stderr, "\t\t-data-reuse-distance-resolution - 0 is equivalent of not using this option\n");
    fprintf(stderr, "\t\t-data-reuse-distance-resolution-final-bin - 0 is equivalent of not using this option\n");
    fprintf(stderr, "\t-analyze-inst-temporal-reuse - activates ITR analysis\n");
    fprintf(stderr, "\t\t-inst-cache-line-size - 0 is equivalent of not using this option\n");
    fprintf(stderr, "\t\t-inst-size - mandatory if the above option is present\n");
    fprintf(stderr, "\t-branch-entropy - activates BE analysis\n");
    fprintf(stderr, "\t-branch-entropy-cond - activates BE analysis only for conditional branches\n");
    fprintf(stderr, "\t-mpi-stats - activates MPI calls analysis\n");
    fprintf(stderr, "\t-mpi-map - activates MPI-map analysis (dump)\n");
    fprintf(stderr, "\t-print-load-store - activates LS analysis\n");
    fprintf(stderr, "\t-register-counting - activates RC analysis\n");
    fprintf(stderr, "\t-mpi-data - activate measurement of data exchanged between the processes\n");
    fprintf(stderr, "\t-external-library-call-count - enable counting external library calls\n");
    fprintf(stderr, "\t-acc - accumulate instructions between the MPI_Tests\n");
    fprintf(stderr, "\t-max-expected-threads - maximum expected threads spanned by the server\n");
    exit(EXIT_FAILURE);
}

static void sigint_handler(int signum) {
    map<int, struct connection_data *>::iterator it;

    if (signum != SIGINT)
        return;

    if (pthread_self() == master_thread) {
        // Wait for all threads to terminate
        for (it = threads_data.begin(); it != threads_data.end(); it++) {
            pthread_join(it->second->thread, NULL);
            //dump_analysis(it->second->thread_id, it->second->JSONwriter.get(), it->second->JSONbuffer.get());
        }

        pthread_exit(NULL);
    }

    pthread_exit(NULL);
}

static void parse_cmd_args(int argc, char **argv) {
    static struct option long_options[] = {
        {"ip", required_argument, 0, 'a'},
        {"portno", required_argument, 0, 'p'},
        {"filename", required_argument, 0, 'f'},
        {"app-name", required_argument, 0, 'j'},
        {"test-name", required_argument, 0, 'k'},
        {"pisa-file-name", required_argument, 0, 'o'},
        {"add-json-data", required_argument, 0, 'g'},
        {"msg-dbg", required_argument, 0, 'm'},
        {"exclude", required_argument, 0, 'e'},
        {"include", required_argument, 0, 'n'},
        {"analyze-data-temporal-reuse", no_argument, 0, 0},
        {"analyze-memory-footprint", no_argument, 0, 0},
        {"data-cache-line-size", required_argument, 0, 'd'},
        {"data-reuse-distance-resolution", required_argument, 0, 'r'},
        {"data-reuse-distance-resolution-final-bin", required_argument, 0, 'b'},
        {"analyze-inst-temporal-reuse", no_argument, 0, 0},
        {"inst-cache-line-size", required_argument, 0, 'i'},
        {"inst-size", required_argument, 0, 's'},
        {"analyze-ilp", no_argument, 0, 0},
        {"ilp-type", required_argument, 0, 't'},
        {"ilp-ctrl", no_argument, 0, 0},
        {"ilp-ignore-ctrl", no_argument, 0, 0},
        {"ilp-verbose", no_argument, 0, 0},
        {"window-size", required_argument, 0, 'w'},
        {"branch-entropy", no_argument, 0, 0},
        {"branch-entropy-cond", no_argument, 0, 0},
        {"mpi-stats", no_argument, 0, 0},
        {"mpi-map", no_argument, 0, 0},
        {"mpi-data", no_argument, 0, 0},
        {"external-library-call-count", no_argument, 0, 0},
        {"register-counting", no_argument, 0, 0},
        {"print-load-store", no_argument, 0, 0},
        {"accumulate", required_argument, 0, 'c'},
        {"max-expected-threads", required_argument, 0, 'x'},
        {0, 0, 0, 0}
    };

    int opt;

    // There are 3 mandatory fields.
    int mandatory = 3;

    while (1) {
        int index = 0;
        opt = getopt_long_only(argc, argv, "a:b:d:e:f:g:i:j:k:m:n:o:p:r:s:t:x:w:", long_options, &index);

        if (opt == -1)
            break;

        switch(opt) {
        case 0:
            if (!strcmp(long_options[index].name, "analyze-ilp"))
                options |= ANALYZE_ILP;
            else if (!strcmp(long_options[index].name, "ilp-ctrl"))
                options |= ANALYZE_ILP_CTRL;
            else if (!strcmp(long_options[index].name, "ilp-ignore-ctrl"))
                options |= ANALYZE_ILP_IGNORE_CTRL;
            else if (!strcmp(long_options[index].name, "ilp-verbose"))
                options |= ANALYZE_ILP_VERBOSE;
            else if (!strcmp(long_options[index].name, "analyze-data-temporal-reuse"))
                options |= ANALYZE_DTR;
            else if (!strcmp(long_options[index].name, "analyze-memory-footprint"))
                options |= ANALYZE_MEM_FOOTPRINT;
            else if (!strcmp(long_options[index].name, "analyze-inst-temporal-reuse"))
                options |= ANALYZE_ITR;
            else if (!strcmp(long_options[index].name, "branch-entropy"))
                options |= PRINT_BRANCH;
            else if (!strcmp(long_options[index].name, "branch-entropy-cond"))
                options |= PRINT_BRANCH_COND;
            else if (!strcmp(long_options[index].name, "register-counting"))
                options |= ANALYZE_REG_COUNT;
            else if (!strcmp(long_options[index].name, "print-load-store"))
                options |= PRINT_LOAD_STORE;
            else if (!strcmp(long_options[index].name, "mpi-map"))
                options |= ANALYZE_MPI_MAP;
            else if (!strcmp(long_options[index].name, "mpi-stats"))
                options |= ANALYZE_MPI_CALLS;
            else if (!strcmp(long_options[index].name, "mpi-data"))
                options |= ANALYZE_MPI_DATA;
            else if (!strcmp(long_options[index].name, "external-library-call-count"))
                options |= ANALYZE_EXTERNALLIBS_CALLS;
            break;
        case 'a':
            ip = strdup(optarg);
            mandatory--;
            break;
        case 'b':
            sscanf(optarg, "%d", &data_reuse_distance_resolution_final_bin);
            break;
        case 'd':
            sscanf(optarg, "%d", &data_cache_line_size);
            break;
        case 'e': {
            char *tokens = strdup(optarg);
            tokens = strtok(tokens, ",");

            while (tokens) {
                ExcludedFunctions.push_back(tokens);
                tokens = strtok(NULL, ",");
            }

            break;
        }
        case 'f':
        {
            parse_ir_file(argv[0], optarg);
            moduleFilename += optarg;
                        moduleExe += optarg;
            mandatory--;
            break;
        }
        case 'i':
            sscanf(optarg, "%d", &inst_cache_line_size);
            break;
        case 'j':
            AppName = strdup(optarg);
            break;
        case 'k':
            TestName = strdup(optarg);
            break;
        case 'm':
            sscanf(optarg, "%d", &debug_flag);
            options |= PRINT_DEBUG;
            break;
        case 'n': {
            char *tokens = strdup(optarg);
            tokens = strtok(tokens, ",");

            while (tokens) {
                IncludeFunctions.push_back(tokens);
                tokens = strtok(NULL, ",");
            }

            break;
        }
        case 'o':
            PISAFileName = strdup(optarg);
            break;
        case 'g':
            AddJSONData = strdup(optarg);
            break;
        case 'p':
            sscanf(optarg, "%d", &portno);
            mandatory--;
            break;
        case 'r':
            sscanf(optarg, "%d", &data_reuse_distance_resolution);
            break;
        case 's':
            sscanf(optarg, "%d", &inst_size);
            break;
        case 't':
            sscanf(optarg, "%d", &ilp_type);
            break;
        case 'w':
            sscanf(optarg, "%d", &window_size);
            break;
        case 'c':
            sscanf(optarg, "%d", &accMode);
            break;
        case 'x':
            sscanf(optarg, "%d", &max_expected_threads);
            break;
        default:
            print_usage(argv[0]);
        }
    }

    if (mandatory)
        print_usage(argv[0]);

    master_thread = pthread_self();

    // Register signal SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    if (options & ANALYZE_MPI_MAP) {
        mpi_map_db.reset(new MPIdb());
        mpi_map_db->s_call.reset(new list<MPIcall>());
        mpi_map_db->r_call.reset(new list<MPIcall>());
        // Initialize all MPIdb members
        // eg: mpi_map_db->allreduce_call = new list<MPIcall>();
    }
}

bool checkStoppingCondition() {
    if(thread_counter <= 0)
        return false;

    if(total_num==0)
        return true;
        
    bool stop = true;
    pthread_mutex_lock(&should_stop_lock);
    
    for (int i = 0 ; i < thread_counter; i++)
        stop = stop && should_stop[i];
    
    pthread_mutex_unlock(&should_stop_lock);

    return stop;
}

int main(int argc, char **argv) {
    weDecoupled = 1;
    //MPI_Init(NULL, NULL);

    parse_cmd_args(argc, argv);
    sockfd = boot_server();

    int epollfd = epoll_create(MAXEVENTS);
    if (epollfd < 0) {
        fprintf(stderr, "Error: epoll_create - %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Initialize epoll event
    struct epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);

    // Buffer where events are returned
    events = (struct epoll_event *)calloc(MAXEVENTS, sizeof(event));

    fprintf(stderr, "Server is ready...\n");

    InitializedThreads = vector<bool>(max_expected_threads, false);
    WKLDcharForThreads = vector<WKLDchar>(max_expected_threads);
    MemoryForThreads = vector<map<Instruction *, void *>>(max_expected_threads);

    SavedStatesForThreads = vector<vector<struct state>>(max_expected_threads);

    // Main loop
    while (!checkStoppingCondition()) {
        int n, i;

        // check every second the stopping condition.
        n = epoll_wait(epollfd, events, MAXEVENTS, 1000); 
        for (i = 0; i < n; i++) {
            //fprintf(stderr,"New event");
            if (events[i].data.fd == sockfd &&
                (events[i].events & EPOLLIN)) {
                // New connection
                accept_new_connections(sockfd, epollfd);
            } else if (events[i].events & EPOLLIN) {
                //fprintf(stderr,"from %d\n", events[i].data.fd);
                if(socket_check(events[i].data.fd) != 0)
                    continue;
                else
                    recv_msg(events[i].data.fd);
            } else {
                // Nothing to be done
                // fprintf(stderr, "Nothing to be done\n");
            }
        }
    }
    
    /*
        map<int, struct connection_data *>::iterator it;
        for (it = threads_data.begin(); it != threads_data.end(); it++) {
            pthread_join(it->second->thread, NULL);
        }
    */
        
    while(total_num>0)
        sched_yield();
            
    return 0;
}
