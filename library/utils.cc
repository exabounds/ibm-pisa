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

#include "utils.h"
#include "WKLDchar.h"

using namespace llvm;

bool weDecoupled=0;
int max_expected_threads = 1;
vector<bool> InitializedThreads;

// Maps between LLVM load/store instructions and real memory addresses
vector<map<Instruction *, void *>> MemoryForThreads;

vector<vector<struct state>> SavedStatesForThreads;
map<void *, Function *> FunctionsAddresses;
static pthread_mutex_t mem2_lock = PTHREAD_MUTEX_INITIALIZER;

// Round double number with 4 decimals
double double4(double x) {
    double result = x;
    unsigned long long precision = 10000;

    result = (double)((unsigned long long)(x * precision)) / precision;
    return result;
}

// This function returns the Function * of a given function id (f)
Module::iterator get_function(int f, Module *M) {
    if (!M)
        return (Module::iterator) NULL;

    int _f = 0;
    
    for (Module::iterator F = M->begin(), N = M->end(); F != N; ++F, ++_f)
        if (_f == f)
            return F;

    return (Module::iterator) NULL;
}

// This function returns the BasicBlock * of a given basic block id (bb)
Function::iterator get_basicblock(int f, int bb, Module *M) {
    if (!M)
        return (Function::iterator) NULL;

    int _bb = 0;
    
    Module::iterator F = get_function(f, M);
    for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB, ++_bb)
        if (_bb == bb)
            return BB;

    return (Function::iterator) NULL;
}

// This function returns the Instruction * of a given instruction id (i)
BasicBlock::iterator get_instruction(int f, int bb, int i, Module *M) {
    if (!M)
        return (BasicBlock::iterator) NULL;

    int _i = 0;
    
    Function::iterator BB = get_basicblock(f, bb, M);
    for (BasicBlock::iterator I = BB->begin(), J = BB->end(); I != J; ++I, ++_i)
        if (_i == i)
            return I;

    return (BasicBlock::iterator) NULL;
}

Function * get_calledFunction(CallInst *call) {
    Function *_call = call->getCalledFunction();

    if (!_call) {
        // Could be also a function called inside a bitcast.
        // So try to extract function from the underlying constant expression.
        // Required to workaround GCC/DragonEGG issue!
        ConstantExpr* expr = dyn_cast<ConstantExpr>(call->getCalledValue());
        if (expr)
            _call = dyn_cast<Function>(expr->getOperand(0));
    }
  
    return _call;
}

bool mpiMatchName(string callName,const char* mpiCallPattern) {
    int len = strlen(mpiCallPattern);
    bool res = !strncasecmp(callName.c_str(),mpiCallPattern,len);

    if (res) {
        char end = callName.c_str()[len];
        res = res && (end == 0 || end == '_');
    }

    return (res);
}

// This function returns the memory address used by the given
// Instruction. The returned address corresponds with the 
// information available for the current thread.
// This function should be called only for load/store
// instructions. Other instructions do not operate with
// memory addresses, so the result will be NULL.
void * getMemoryAddress(Instruction *I, const int thread_id) {
    void *value = NULL;

    if (thread_id < max_expected_threads) {
        if (InitializedThreads[thread_id]) {
            const auto it = MemoryForThreads[thread_id].find(I);
            if (it !=  MemoryForThreads[thread_id].end())
                value = it->second;
        }
    }

    return value;
}

// This function returns the upper power of 2 for a given value.
// It is used during the DTR analysis.
unsigned long long upperPowerOfTwo(unsigned long long v)
{
    if (!v)
        return v;
    
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;

    return v;
}

pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;

// This function returns the 'Function *', specific to LLVM, 
// for a given real memory address.
Function * getFunctionAddress(void *addr)
{
    Function *F = NULL;
    std::map<void *, Function *>::iterator it;

    pthread_mutex_lock(&m_lock);
    it = FunctionsAddresses.find(addr);
    if (it != FunctionsAddresses.end())
        F = it->second;
    pthread_mutex_unlock(&m_lock);

    return F;
}

// Comparator used by the set_intersection function
bool map_less(std::pair<const unsigned long long, bool> &v1, 
              std::pair<const unsigned long long, bool> &v2) {
    return v1.first < v2.first;
}

bool map_less_(std::pair<const unsigned long long, unsigned long long> &v1, 
               std::pair<const unsigned long long, unsigned long long> &v2) {
    return v1.first < v2.first;
}

// Function used to find the shared addresses between two sets of sorted keys
void intersectTwoMaps(map<unsigned long long, bool> &m1, 
                      map<unsigned long long, bool> &m2,
                      std::vector<std::pair<unsigned long long, bool> > &output) {
    auto it = set_intersection(m1.begin(), m1.end(), m2.begin(), m2.end(), output.begin(), map_less);
    output.resize(it-output.begin());
}

void intersectTwoMapsLong(map<unsigned long long, unsigned long long> &m1, 
                          map<unsigned long long, unsigned long long> &m2,
                          std::vector<std::pair<unsigned long long, unsigned long long> > &output) {
    auto it = set_intersection(m1.begin(), m1.end(), m2.begin(), m2.end(), output.begin(), map_less_);
    output.resize(it-output.begin());
}

// Function used to find the shared addresses when there are more than two sets of sorted keys
void intersectMore(std::vector<std::pair<unsigned long long, bool> >& crtOutputOrig, 
                   map<unsigned long long, bool> &m,
                   std::vector<std::pair<unsigned long long, bool> > &output) {
    std::vector<std::pair<const unsigned long long, bool> >& crtOutput = 
               (std::vector<std::pair<const unsigned long long, bool> >&)crtOutputOrig;
    auto it = set_intersection(crtOutput.begin(), crtOutput.end(), m.begin(), m.end(), output.begin(), map_less);
    output.resize(it-output.begin());
}

void intersectMoreLong(std::vector<std::pair<unsigned long long, unsigned long long> >& crtOutputOrig, 
                       map<unsigned long long, unsigned long long> &m,
                       std::vector<std::pair<unsigned long long, unsigned long long> > &output) {
    std::vector<std::pair<const unsigned long long, unsigned long long> >& crtOutput = 
               (std::vector<std::pair<const unsigned long long, unsigned long long> >&)crtOutputOrig;
    auto it = set_intersection(crtOutput.begin(), crtOutput.end(), m.begin(), m.end(), output.begin(), map_less_);
    output.resize(it-output.begin());
}

int is_mpi_sync_call(Function *_call)
{
    std::string name = _call->getName().str();
    
    if(name.back() == '_' )
        name.pop_back();
    
    const char* cname = name.c_str();

    if (!strcasecmp(cname,"MPI_Send") ||
        !strcasecmp(cname,"MPI_Recv") ||
        !strcasecmp(cname,"MPI_Sendrecv") ||
        !strcasecmp(cname,"MPI_Sendrecv_replace") ||
        !strcasecmp(cname,"MPI_Isend") ||
        !strcasecmp(cname,"MPI_Irecv") ||
        !strcasecmp(cname,"MPI_Wait") ||
        !strcasecmp(cname,"MPI_Test") ||
        !strcasecmp(cname,"MPI_Bcast") ||
        !strcasecmp(cname,"MPI_Reduce") ||
        !strcasecmp(cname,"MPI_Gather") ||
        !strcasecmp(cname,"MPI_Gatherv") ||
        !strcasecmp(cname,"MPI_Allreduce") ||
        !strcasecmp(cname,"MPI_Allgather") ||
        !strcasecmp(cname,"MPI_Allgatherv") ||
        !strcasecmp(cname,"MPI_Alltoall") ||
        !strcasecmp(cname,"MPI_Alltoallv") ||
        !strcasecmp(cname,"MPI_Scatter") ||
        !strcasecmp(cname,"MPI_Scatterv") )
        return 1;

    return 0;
}
