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

#include <cstdio>
#include <iostream>
#include <map>
#include <memory>

#include <mpi.h>
#include <omp.h>
#include <pthread.h>

#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>

#include "JSONmanager.h"
#include "utils.h"
#include "InstructionAnalysis.h"
#include "WKLDchar.h"
#include "mpi_sync_server.h"
#include "MPIcnfSupport.h"

#include "llvm/IR/LLVMContext.h"
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

using namespace std;

/*
    TODO: DEBUG!!!
    This can generate leakage in theory but the module M is created once and then only always read.
    There seems to be a problem with multiple threads when they are exiting they tend to free the unique pointer multiple time.
    This problem might be related to something different than multi-threaded but still, it might generate segmentation fault on exit.
    I prefer not to free this memory rather than segment because we need to load once at the very beginning and free only on exit.

    std::unique_ptr<Module> M;
*/

struct noDestroy_Functor {
    void operator() (Module *m) const
    {
    //    if (total_num == 0 && m != NULL)
    //      delete m;
        return;
    }
    void operator() (LLVMContext *m) const
    {
    //    if (total_num == 0 && m != NULL)
    //      delete m;
        return;
    }
};

//std::unique_ptr<LLVMContext,noDestroy_Functor> context;
std::unique_ptr<LLVMContext> context;

//std::unique_ptr<Module,noDestroy_Functor> M;
std::unique_ptr<Module> M;

extern int max_expected_threads; // Instantiated in utils.cc
int processor_id = 0;
int options = 0;
int data_cache_line_size = 0;
int data_reuse_distance_resolution = 0;
int data_reuse_distance_resolution_final_bin = 0;
int inst_cache_line_size = 0;
int inst_size = 0;
int window_size = 0;
int ilp_type = 0;
int debug_flag = 0;
bool mpi_map_active = false;

char *AppName;
char *TestName;
char *PISAFileName;
char *AddJSONData;
char *branch_entropy_file;


// This flag is used to memorize the LLVM address (the register)
// of the last flag inside a MPI_Test call
Value *mpi_flag = NULL;

// If this flag is set, all the instructions must be ignored
// from our analysis. FIXME: it looks like always false! And not modified by the pass...
bool mpi_ignore;

// FIXME: replace with vector to avoid the locks
/*std::vector<pthread_mutex_t> LocksForThreads(kMaxThreads, PTHREAD_MUTEX_INITIALIZER);
static pthread_mutex_t locksThreads_lock = PTHREAD_MUTEX_INITIALIZER;*/
static pthread_mutex_t module_lock = PTHREAD_MUTEX_INITIALIZER;

/// Map between threads and wkld characterisations

// Maps between LLVM load/store instructions and real memory addresses
//map<int, map<Instruction *, void *> *> MemoryForThreads;// This has been moved to utils.cc and utils.h :|

vector<WKLDchar>  WKLDcharForThreads;
vector<bool> InitializedMPIThreads;
vector<bool> InitializedMPISockets;

// Map between threads and sockets id that represents the connection with the MPI server
vector<int> MPISocketForThreads;

// Maps for saving the mapping between a real address and an MPI_db info
vector<map<void *, struct MPIinfo>> MPIInfoForThreads;

// Vector with the names of the functions that must be excluded from analysis
vector<string> ExcludedFunctions;
// e_lock is used to sync access to ExcludedFunctions
static pthread_mutex_t e_lock = PTHREAD_MUTEX_INITIALIZER;

// Vector with the names of the functions that must be analysed
vector<string> IncludeFunctions;
// i_lock is used to sync access to IncludeFunctions
static pthread_mutex_t i_lock = PTHREAD_MUTEX_INITIALIZER;

// This lock is used to sync access for dumping information.
// For example, during the LoadStoreVerbose analysis, the
// information is printed dynamically and it is not collected
// in the memory. All threads performs I/O operations
// so the printing process must be mutual exclusive done.
static pthread_mutex_t ls_lock = PTHREAD_MUTEX_INITIALIZER;

bool mpiExecution = false;
static pthread_mutex_t ilp_dbg_output_lock = PTHREAD_MUTEX_INITIALIZER;

// This function updates the mapping for the given Instruction.
// It maps the Instruction with a memory address, for the
// current thread. This function should be called only
// for load/store instructions. Other instructions don't
// operate with memory addresses, so the mapping would not be
// useful.
void updateMemory(Instruction *I, void *value, const int thread_id) {
    if (thread_id < max_expected_threads) {
        if (InitializedThreads[thread_id]) {
            MemoryForThreads[thread_id][I] = value;
            /*
            auto it = MemoryForThreads[thread_id].find(I);
            if (it != MemoryForThreads[thread_id].end())
                it->second = value;
            else
                MemoryForThreads[thread_id].insert(pair<Instruction *, void *>(I, value));
            */
        }
    }
}


// This function verifies is a basic block is inside a given function
static bool check_if_bb_is_in_function(Function *F, Function::iterator BB_i) {
    if (!M)
        return false;

    for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
        if (BB == BB_i)
            return true;

    return false;
}

#define DEBUG_END_APP 0
// This function is called if a return instruction is encounter inside the
// 'main' function. It dumps the data collected by the framework.
extern "C" void end_app(void) {
    // When using named semaphores, from time to time, the output is still mixed up (even if it happens rarely.
    // We synchronize using MPI and generate an in-order output, this should make it easier for debugging (or even solving it).
    /*
        if(ls_semaphor && ls_semaphor != SEM_FAILED)
            sem_wait(ls_semaphor);
    */
    int procWriting = 0;
    int procWrote = -1;
    int procId = 0;
    int procCount = 1;
    int masterId = 0;

    if(mpiExecution){
        MPI_Comm_rank(MPI_COMM_WORLD, &procId);
        MPI_Comm_size(MPI_COMM_WORLD, &procCount);
    }

    JSONmanager JSONman(procId, options, &WKLDcharForThreads);	
    if(procId==0){
        #if DEBUG_END_APP
            fprintf(stderr,"libanalysisCoupled: Process %d opening JSON as master\n",procId);
        #endif
        JSONman.openMaster(PISAFileName, AddJSONData, AppName, TestName);
    }
    else{
        #if DEBUG_END_APP
            fprintf(stderr,"libanalysisCoupled: Process %d opening JSON as slave\n",procId);
        #endif
        JSONman.openSlave(masterId);// FIXME: parameters masterId
    }
        
    for(procWriting=0 ; procWriting < procCount ; procWriting++){
        if(procWriting == procId){// Writing turn of this process
            procWrote = procId;
        }// Writing completed
        
        // In this function the JSONman master either dump his own JSON or it sets in receiving mode listening for the slave
        // The JSONman slaves either send data to the master or do nothing.
        JSONman.dump(procWriting);
        
        if(mpiExecution){
            #if DEBUG_END_APP
                fprintf(stderr,"libanalysisCoupled: Process %d entering Bcast synchronization. procWriting %d, procWrote %d\n", procId, procWriting, procWrote);
            #endif
        MPI_Bcast(&procWrote, 1, MPI_INT, procWriting, MPI_COMM_WORLD);// Synchronizing
            #if DEBUG_END_APP
                fprintf(stderr,"libanalysisCoupled: Process %d exiting Bcast synchronization. procWriting %d, procWrote %d\n", procId, procWriting, procWrote);
            #endif
        assert(procWrote == procWriting);
      }
    }

    JSONman.close();

    if(mpiExecution){
        MPI_Finalize();
    }
    /*    
    if(ls_semaphor && ls_semaphor != SEM_FAILED)
        sem_post(ls_semaphor);
    //    sem_close(ls_semaphor);
    */
}

// This function gets specific per thread information.
static void get_per_thread_info(const int thread_id) {
    assert(thread_id < max_expected_threads);

    if (!InitializedThreads[thread_id]) {
        InitializedThreads[thread_id] = true;
    }
    
}


/*static void get_mpi_thread_map_info(const int thread_id) {
    if (thread_id >= max_expected_threads) {
        //end_app();
        exit(EXIT_FAILURE);
    }

    get_per_thread_info(thread_id);
    MPI_Comm_rank(MPI_COMM_WORLD, &processor_id);
    WKLDcharForThreads[thread_id].update_processor_id(processor_id);

    if (!InitializedMPIThreads[thread_id]) {
        MPIInfoForThreads[thread_id] = map<void *, struct MPIinfo>();
        InitializedMPIThreads[thread_id] = true;
    }
}*/

// This function process an instruction
static void process_instr(Instruction *I, const int thread_id) {
    // 'analyze' function is responsible for calling the
    // 'visit' function for each active analysis
    if (!mpi_ignore)
        WKLDcharForThreads[thread_id].analyze(*I);

    // If the current instruction is a call to a function
    // named 'exit', the application will end so we need
    // to dump all the extracted information by manually
    // calling 'end_app' function.
    if (strcmp(I->getOpcodeName(), "phi")) {
        if (!strcmp(I->getOpcodeName(), "call")) {
            CallInst *call = cast<CallInst>(I);
            Function *F = get_calledFunction(call);

            if (F && F->getName().str() == "exit")
                end_app();
        }
    }
}

// SavedStates is a vector that memories the call trace.
// This function verifies if the current basic block
// is inside an included function for the analysis
// or the SavedStates contains such a function.
static bool check_if_bb_is_in_included_functions(Function::iterator BB, const int thread_id) {
    if (!M)
        return false;

    if (IncludeFunctions.size() == 0)
        return true;

    pthread_mutex_lock(&i_lock);

    for (unsigned i = 0; i < IncludeFunctions.size(); i++) {
        Function *F = M->getFunction(IncludeFunctions[i]);
        if (F && check_if_bb_is_in_function(F, BB)) {
            pthread_mutex_unlock(&i_lock);
            return true;
        }
    }

    for (unsigned i = 0; i < IncludeFunctions.size(); i++) {
        Function *F = M->getFunction(IncludeFunctions[i]);
        for (unsigned j = 0; j < SavedStatesForThreads[thread_id].size(); j++) {
            struct state previous = SavedStatesForThreads[thread_id][j];
            if (F && check_if_bb_is_in_function(F, previous.BB)) {
                pthread_mutex_unlock(&i_lock);
                return true;
            }
        }
    }

    pthread_mutex_unlock(&i_lock);

    return false;
}

// SavedStates is a vector that memories the call trace.
// This function verifies if the current basic block
// is inside an excluded function for the analysis
// of the SavedStates contains such a function.
static bool check_if_bb_is_in_excluded_functions(Function::iterator BB, const int thread_id) {
    if (!M || ExcludedFunctions.size() == 0)
        return false;

    pthread_mutex_lock(&e_lock);

    for (unsigned i = 0; i < ExcludedFunctions.size(); i++) {
        Function *F = M->getFunction(ExcludedFunctions[i]);
        if (F && check_if_bb_is_in_function(F, BB)) {
            pthread_mutex_unlock(&e_lock);
            return true;
        }
    }

    pthread_mutex_unlock(&e_lock);

    if (SavedStatesForThreads[thread_id].size() == 0)
        return false;

    /* Also check in SavedStates */
    pthread_mutex_lock(&e_lock);

    for (unsigned i = 0; i < ExcludedFunctions.size(); i++) {
        Function *F = M->getFunction(ExcludedFunctions[i]);
        for (unsigned j = 0; j < SavedStatesForThreads[thread_id].size(); j++) {
            struct state previous = SavedStatesForThreads[thread_id][j];
            if (F && check_if_bb_is_in_function(F, previous.BB)) {
                pthread_mutex_unlock(&e_lock);
                return true;
            }
        }
    }

    pthread_mutex_unlock(&e_lock);

    return false;
}

// This function is responsible of processing a basic block, starting
// from the I_init instruction. The procesing is done until the end
// of the basic block or until is interrupted because it found
// a call to an internal function or a load/store instruction.
static void iter_instructions(Function::iterator BB, BasicBlock::iterator I_init, const int thread_id) {
    Instruction *last_processed_inst = NULL;

    get_per_thread_info(thread_id);

    for (BasicBlock::iterator I = I_init, J = BB->end(); I != J; ++I) {
        Function *_call = NULL;
        if (!strcmp(I->getOpcodeName(), "call")) {
            CallInst *call = cast<CallInst>(I);
            _call = get_calledFunction(call);

            if (!_call) {// FIXME: Giovanni question: why this part does not match the one in server.cc? Is there a bug (here or in server.cc)?
                Value * a = (Function *)call->getCalledValue();
                void *addr = getMemoryAddress((Instruction *)a, thread_id);
                if (addr)
                    addr = (void *)*(unsigned long long *)addr;
                _call = getFunctionAddress(addr);
            }

            // We are ignoring LLVM's debugging functions
            if (_call && !strncmp(_call->getName().str().c_str(), "llvm.", 5))
                continue;

    #if 0
            // This sets mpi_flag. The flag is used to ignore everything
            // between failing MPI_Test calls. Uncomment this to
            // enable the functionality. The rest of the implementation
            // is used only if mpi_flag is set. If no one enables the flag,
            // than the entire feature is disabled.
            if (_call && !strncasecmp(_call->getName().str().c_str(), "MPI_Test",8)) {
                CallInst *CI = cast<CallInst>(I);
                mpi_flag = CI->getArgOperand(1);
            }
    #endif
        }

        // If the current instruction is a load/store instruction, the
        // processing will be interrupt and will be restored by the
        // 'update_var' call, after the real memory address is updated.
        if (!strcmp(I->getOpcodeName(), "load") || !strcmp(I->getOpcodeName(), "store"))
            break;

        // The instruction is processed only if it's contained in a
        // included function and it is not contained in a excluded function.
        if (check_if_bb_is_in_included_functions(BB, thread_id))
            if (!check_if_bb_is_in_excluded_functions(BB, thread_id))
                process_instr((Instruction*)I, thread_id);

        // We memorized the last processed instruction
        last_processed_inst = (Instruction*)I;

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
                SavedStatesForThreads[thread_id].push_back(current_state);
                return;
            }
          
            // performing MPI_sync operation
            if (_call && is_mpi_sync_call(_call))
                return;
            //}
            // TODO: If we get here, we are calling an external function.
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
        if (options & ANALYZE_ILP) {
            // The 'call' instruction is a special instruction for ILP.
            // We should update the IssueCycle of the instruction
            // (ergo the IssueCycle of the variable that gets the result
            // of the call) to be the same as it was for the return
            // instruction.
            next_state.I--;
            WKLDcharForThreads[thread_id].updateILPforCall(last_processed_inst, (Instruction*)next_state.I);
            next_state.I++;
        }
        iter_instructions(next_state.BB, next_state.I, thread_id);
    }
}

// This is a library call that notifies us that to perform the
// analysis of the bb basic block.
extern "C" void inst_notification(int f, int bb) {
    if (!M)
        return;

    Function::iterator BB = get_basicblock(f, bb, M.get());
    iter_instructions(BB, BB->begin(), omp_get_thread_num());
}

static void sigint_handler(int signum) {
    if (signum != SIGINT)
        return;

    end_app();
    exit(1);
}

extern "C" void app_name(char *s) {
    AppName = s;
}

extern "C" void update_branch_entropy_file(char *s) {
    branch_entropy_file = s;
}

extern "C" void test_name(char *s) {
    TestName = s;
}

extern "C" void pisa_output(char *s) {
    if(strlen(s)>0)
        PISAFileName = s;
    else
        PISAFileName = getenv("PISAFileName");
}

extern "C" void add_json_data(char *s) {
    if(strlen(s)>0)
        AddJSONData = s;
    else
        AddJSONData = getenv("AddJSONData");
}

extern "C" void update_max_expected_threads(int size) {
    WKLDcharForThreads = vector<WKLDchar>(size);
    MemoryForThreads = vector<map<Instruction *, void *>>(size);
    SavedStatesForThreads = vector<vector<struct state>>(size);
    InitializedThreads = vector<bool>(size, false);
    InitializedMPIThreads = vector<bool>(size, false);
    InitializedMPISockets = vector<bool>(size, false);
    MPISocketForThreads = vector<int>(size, 0);
    MPIInfoForThreads = vector<map<void *, struct MPIinfo>>(size);
    max_expected_threads = size;
}

// This function initialize the libanalysis. Calls different constructors
// and parse the LLVM IR string.
extern "C" void init_libanalysis(char *s, int flags) {
    /*
        Initialize named semaphore for output.
        Note that if multiple profiling are running in parallel,
        this hard coded semaphore naming would share a single semaphore along all profiling runs.
        Printing output will happen in serialized way regardless the number of parallel experiments.
    */

    // Register signal SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_flags = SA_RESETHAND; /* restore after */
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    // All instruction must be analyzed.
    mpi_ignore = false;
    // There was no previos MPI_Test call
    mpi_flag = NULL;

    pthread_mutex_lock(&module_lock);
    if (M.get() == NULL) {
    //  M = std::unique_ptr<Module>(new Module("TempModule", (LLVMContext&)context));
    //  context = std::unique_ptr<LLVMContext,noDestroy_Functor>(new LLVMContext);
        context = std::unique_ptr<LLVMContext>(new LLVMContext);
        
    //  M = std::unique_ptr<Module>(new Module("TempModule", *(context.get()) ));
    //  M = std::unique_ptr<Module,noDestroy_Functor>(new Module("TempModule", *(context.get())));
        M = std::unique_ptr<Module>(new Module("TempModule", *(context.get())));
    }
    pthread_mutex_unlock(&module_lock);

    SMDiagnostic errs;

#if LLVM_VERSION_MINOR >= 7
    M = parseAssemblyString(s, errs, M->getContext());
    if(M == nullptr) {
#else
    if (!ParseAssemblyString(s, M.get(), errs, M->getContext())) {
#endif
        std::string errMsg;
        raw_string_ostream os(errMsg);
        errs.print("", os);
        std::cerr << "Error parsing " << s << "\n";
        std::cerr << os.str();
    }

    options = flags;

    /*
        In an OpenMP application, init_libanalysis is executed only by the master thread because
        it is put as very first instruction in the main that is not in a parallel section.
        Still we need to inizialize all threads!
    */
    int tid;
    #pragma omp parallel private(tid)
    {
        tid = omp_get_thread_num();
        // The master thread initializes all WKLDchar
        if (tid == 0) {
            for(int thread_id = 0; thread_id < omp_get_num_threads(); thread_id++){
                if (thread_id < max_expected_threads) {

                    /*
                        Since PISA is compled using OpenMP, I will initialize the data structure even for non-OMP applications.
                        Nonetheless I will set: InitializedThreads[thread_id] to true only the first time the application actually
                        uses thread_id.
                    */
                    //  if (!InitializedThreads[thread_id]) {
                    WKLDcharForThreads[thread_id] = WKLDchar(M.get(), 
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
                                                             processor_id, 
                                                             &ls_lock, 
                                                             &ilp_dbg_output_lock, 
                                                             branch_entropy_file);
                    MemoryForThreads[thread_id] = map<Instruction *, void *>();
                    SavedStatesForThreads[thread_id] = vector<struct state>();
                }
            }
        }
    }
}

// This function updates the vector of ExcludedFunctions
extern "C" void exclude_function(char *s) {
    pthread_mutex_lock(&e_lock);
    ExcludedFunctions.push_back(s);
    pthread_mutex_unlock(&e_lock);
}

// This function updates the vector of IncludeFunctions
extern "C" void include_function(char *s) {
    pthread_mutex_lock(&i_lock);
    IncludeFunctions.push_back(s);
    pthread_mutex_unlock(&i_lock);
}


// This function receives the real memory address of the function
// with the id 'f' and updates the FunctionsAddresses mapping.
extern "C" void func_address(int f, ...) {
    va_list argp;
    Module::iterator F = get_function(f, M.get());
    va_start(argp, f);
    void *value = va_arg(argp, void *);

    pthread_mutex_lock(&m_lock);
    /* std::map<void *, Function *>::iterator it;
    it = FunctionsAddresses.find(value);
    if (it != FunctionsAddresses.end())
        FunctionsAddresses.erase(it);
    FunctionsAddresses.insert(std::pair<void *, Function *>(value, F));*/
    
    FunctionsAddresses[value] = (Function*)F;
    pthread_mutex_unlock(&m_lock);
    va_end(argp);
}

// This function updates the data cache line size
extern "C" void update_data_cache_line_size(int size) {
    data_cache_line_size = size;
}

// This function updates the data cache line size
extern "C" void update_data_reuse_distance_resolution(int size) {
    data_reuse_distance_resolution = size;
}

// This function updates the data cache line size
extern "C" void update_data_reuse_distance_resolution_final_bin(int size) {
    data_reuse_distance_resolution_final_bin = size;
}

// This function updates the instruction cache line size
extern "C" void update_inst_cache_line_size(int size) {
    inst_cache_line_size = size;
}

// This function updates the instruction size
extern "C" void update_inst_size(int size) {
    inst_size = size;
}

extern "C" void update_ilp_type(int type) {
    ilp_type = type;
}

extern "C" void enable_debug(int enableFlag) {
    debug_flag = enableFlag;
}

extern "C" void update_window_size(int windowSize) {
    window_size = windowSize;
}

static void update_real_memory_issue_cycle(Value *reg, void *real_addr, int type_op, unsigned long long issue_cycle, const int thread_id) {
    get_per_thread_info(thread_id);
    WKLDcharForThreads[thread_id].updateILPIssueCycle(type_op, reg, real_addr, issue_cycle);
}

static void set_ILP_issue_cycle(MPIcall *message, int which_buffer, MPIupdate *update, Value *ret, unsigned long long issue_cycle, const int thread_id) {
    void *real_addr = update->s_buf;
    Value *mpi_value = (Value *)update->s_value;
    int count = update->s_count;
    int *vcount = update->s_vcount;
    int *depl = update->s_depl;
    MPI_Datatype dtype = update->s_dtype;
    int other_id = message->id_r;

    if (which_buffer & SECOND_RECV_BUFFER) {
        real_addr = update->r_buf;
        mpi_value = (Value *)update->r_value;
        count = update->r_count;
        vcount = update->r_vcount;
        depl = update->r_depl;
        dtype = update->r_dtype;
        other_id = message->id_s;
    }

    get_per_thread_info(thread_id);

    int i;
    int dsize = 0;
    MPI_Type_size(dtype, &dsize);
    int type_op = 0;

    if (message->type == GET_RECV_INFO) {
        // The current process is doing a MPI_Send call.
        // The data is performing a read operation.
        type_op = READ_OPERATION;
    } else if (message->type == GET_SEND_INFO) {
        // The current process is doing a MPI_Recv call.
        // The data is performing a write operation.
        type_op = WRITE_OPERATION;
    }

    if (depl && vcount) {
        int size_comm = 0;
        MPI_Comm_size(message->comm, &size_comm);
        for (i = 0; i < size_comm; i++) {
            char *real_offset_addr = (char *)real_addr;
            real_offset_addr += depl[i] * dsize;
            int j;
            for (j = 0; j < vcount[i]; j++)
                WKLDcharForThreads[thread_id].updateILPIssueCycle(type_op, mpi_value, real_offset_addr + j * dsize, issue_cycle);
        }
    } else {
        char *real_offset_addr = (char *)real_addr;
        for (i = 0; i < count; i++) {
            WKLDcharForThreads[thread_id].updateILPIssueCycle(type_op, mpi_value, real_offset_addr + i * dsize, issue_cycle);
        }
    }

    // Also update the Value * of the register that contains the returned
    // value of the MPI_Call
    WKLDcharForThreads[thread_id].updateILPforCall(mpi_value, ret);
}

// This functions returns the socket id that represents the
// connection between the current thread and the MPI server.
/*
static int mpi_sync_get_socket_id(const int thread_id) {
    int sockfd = -1;

    if (!InitializedMPISockets[thread_id]) {
        fprintf(stderr, "Error: Can not find socket. This should not happen.\n");
        exit(EXIT_FAILURE);

    } else {
        sockfd = MPISocketForThreads[thread_id];
    }

    return sockfd;
}
*/

// This functions receives the IP and PORT number of the server
// in the following format: $IP_ADDRESS:$PORT
// The function parse the string to extract the info and
// opens the connection with the server. This connection
// will be used inside the mpi_update_db function
// in order to transmit/receive messages with/from the server.
extern "C" void mpi_sync_server_info(char *info) {
    int portno = -1;
    int sockfd = -1;
    char *ip = strtok(strdup(info), ":");

    sscanf(strtok(NULL, ":"), "%d", &portno);
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

    const int thread_id = omp_get_thread_num();
    if (!InitializedMPISockets[thread_id]) {
        MPISocketForThreads[thread_id] = sockfd;
        InitializedMPISockets[thread_id] = true;
    } else {
        fprintf(stderr, "Error: thread %d already has a socket\n", thread_id);
        exit(EXIT_FAILURE);
    }
}

static void send_msg_to_db(int sockfd, MPIcall *msg) {
    int n = write(sockfd, msg, sizeof(MPIcall));
    if (n < 0) {
        fprintf(stderr, "Error: writing to socket\n");
        exit(EXIT_FAILURE);
    }
}

static void recv_msg_from_db(int sockfd, MPIcall *msg) {
    int n = read(sockfd, msg, sizeof(MPIcall));
    if (n < 0) {
        fprintf(stderr, "Error: reading from socket\n");
        exit(EXIT_FAILURE);
    }
}

static void send_query_and_update(int sockfd, MPIcall *message, Value *I, MPIupdate *update, int which_buffer, const int thread_id) {
    // Send query
    while (1) {
        send_msg_to_db(sockfd, message);

        MPIcall msg_recv;
        memset(&msg_recv, 0, sizeof(MPIcall));
        recv_msg_from_db(sockfd, &msg_recv);
        
        if (msg_recv.type != INFO_NOT_FOUND) {
            // Server transmited a proper response. Update the issue_cycle.
            // If mpi_map_active is set, then we are performing mpi-map analysis, not mpi-sync.
            // The issuce cycle does not need to be updated.
            if (!mpi_map_active)
                set_ILP_issue_cycle(message, which_buffer, update, I, msg_recv.issue_cycle, thread_id);
            else {
                // errs() << "type,current_index,diff,other_id,other_index,action_index,completion_status\n";
                // Function *_call = get_calledFunction((CallInst *)update->call);

                if (message->type == GET_SEND_INFO)
                    errs() << update->index << ",r," << update->dif << "," << message->id_s << "," << msg_recv.inst << ",,,\n";
                else if (message->type == GET_RECV_INFO)
                    errs() << update->index << ",s," << update->dif << "," << message->id_r << "," << msg_recv.inst << ",,,\n";
            }
            break;
        }
    }
}

/*
static void send_query_and_update_both_buf(int sockfd, MPIcall *message, MPIupdate *update, const int thread_id) {
    // Send query
    while (1) {
        send_msg_to_db(sockfd, message);

        MPIcall msg_recv;
        memset(&msg_recv, 0, sizeof(MPIcall));
        recv_msg_from_db(sockfd, &msg_recv);
        if (msg_recv.type != INFO_NOT_FOUND) {
            update_real_memory_issue_cycle((Value *)update->s_value, update->s_buf, READ_OPERATION, msg_recv.issue_cycle, thread_id);
            update_real_memory_issue_cycle((Value *)update->r_value, update->r_buf, WRITE_OPERATION, msg_recv.issue_cycle, thread_id);
            update_real_memory_issue_cycle((Value *)update->r_value, update->r_buf, READ_OPERATION, msg_recv.issue_cycle, thread_id);

            break;
        }
    }
}

static void send_msg_to_all_comm(int sockfd, MPIcall *message) {
    int size, i;
    MPI_Comm_size(message->comm, &size);

    // This is the root node it should send (sizeof(COM) - 1) messages
    for (i = 0; i < size; i++)
        if (message->type == PUT_SEND_INFO && i != message->id_s) {
            message->id_r = i;
            send_msg_to_db(sockfd, message);
        } else if (message->type == PUT_RECV_INFO && i != message->id_r) {
            message->id_s = i;
            send_msg_to_db(sockfd, message);
        }

    if (message->type == PUT_SEND_INFO)
        message->id_r = message->id_s;
    else if (message->type == PUT_RECV_INFO)
        message->id_s = message->id_r;
}

static void send_query_and_update_comm(int sockfd, MPIcall *message, Value *I, MPIupdate *update, int which_buffer, const int thread_id) {
    int size, i;
    MPI_Comm_size(message->comm, &size);

    // This is the root node it should send (sizeof(COM) - 1) messages
    for (i = 0; i < size; i++)
        if (message->type == GET_RECV_INFO && i != message->id_s) {
            message->id_r = i;
            send_query_and_update(sockfd, message, I, update, which_buffer, thread_id);
        } else if (message->type == GET_SEND_INFO && i != message->id_r) {
            message->id_s = i;
            send_query_and_update(sockfd, message, I, update, which_buffer, thread_id);
        }

    if (message->type == GET_RECV_INFO)
        message->id_r = message->id_s;
    else if (message->type == GET_SEND_INFO)
        message->id_s = message->id_r;
}
*/

extern "C" void mpi_map_server_info(char *info) {
    mpi_map_active = true;
    errs() << "current_index,type,num_inst_diff,other_id,other_index,action_index,completion_status\n";
    mpi_sync_server_info(info);
}

// This function receives the real memory address of a given
// load/store instruction. It updates the mapping and
// then it continues the processing of the current basic block,
// starting from the load/store instruction.
extern "C" void update_vars(int f, int bb, int i, ...) {
    // pthread_mutex_t threadLock = getLock();
    // pthread_mutex_lock(&threadLock);

    va_list argp;
    Function::iterator BB = get_basicblock(f, bb, M.get());
    BasicBlock::iterator I = get_instruction(f,bb, i, M.get());

    if (!M || !((Instruction*)I)) {
        // pthread_mutex_unlock(&threadLock);
        return;
    }

    va_start(argp, i);
    void *value = va_arg(argp, void *);
    va_end(argp);

    // WKLDchar *wkld = NULL;
    // std::vector<struct state> *SavedStates = NULL;
    const int thread_id = omp_get_thread_num();
    get_per_thread_info(thread_id);

    if ( (options & PRINT_DEBUG) && mpiExecution) {
        // DEBUG INFO
        int procId;
        MPI_Comm_rank(MPI_COMM_WORLD, &procId);
        if(procId == 1){
            I->dump();
            cerr << "Memory address " << (unsigned long long *) value << endl;
        }
    }

    if (!mpi_ignore) {
        updateMemory((Instruction*)I, value, thread_id);
    }

    // if bb is in included functions, but not in exclude
    if (check_if_bb_is_in_included_functions(BB, thread_id))
        if (!check_if_bb_is_in_excluded_functions(BB, thread_id))
            process_instr((Instruction*)I, thread_id);

    I++;

    if (I != BB->end())
        iter_instructions(BB, I, thread_id);

    //  pthread_mutex_unlock(&threadLock);
}

void process_msg(struct message *msg){
    Function::iterator BB = get_basicblock(msg->data.mpi.f_id, msg->data.mpi.bb_id, M.get());
    BasicBlock::iterator I = get_instruction(msg->data.mpi.f_id,
                         msg->data.mpi.bb_id,
                         msg->data.mpi.i_id, M.get());
                         
    const int thread_id = omp_get_thread_num();

    if (options & ANALYZE_MPI_DATA)
        WKLDcharForThreads[thread_id].process_mpi_data((Instruction*)I, msg);

    I++;

    if (I != BB->end())
        iter_instructions(BB, I, thread_id);
}


/*
    This is executed with MPI_Init.
    Using MPI+OpenMP it is very likely that only one OpenMP thread executes the initialization for the MPI process.
    The processor_id must be updated for all OpenMP threads.
*/
extern "C" void mpi_update_process_id(void) {
    int procId, tid;
    mpiExecution = true;

    MPI_Comm_rank(MPI_COMM_WORLD, &procId);

    // We let the master thread initialize all procId. 
    // When not in a parallel section, omp_get_num_threads returns 1.
    #pragma omp parallel private(tid)
    {
        tid = omp_get_thread_num();
        // The master thread initializes all processIds
        if (tid == 0) 
        {
            for(int thread_id = 0; thread_id < omp_get_num_threads(); thread_id++){
                WKLDcharForThreads[thread_id].update_processor_id(procId);
            }
        }
    }
}

