class OpenMPstats;

#ifndef LLVM_OPEN_MP_STATS__H
#define LLVM_OPEN_MP_STATS__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "JSONmanager.h"
#include "splay.h"

class OpenMPstats: public InstructionAnalysis {
    unsigned long long NumAtomic;

    unsigned long long NumStartupShutdown;
    unsigned long long NumBegin;
    unsigned long long NumEnd;

    unsigned long long NumParallel;
    unsigned long long NumPushNumThreads;
    unsigned long long NumForkCall;
    unsigned long long NumPushNumTeams;
    unsigned long long NumForkTeams;
    unsigned long long NumSerializedParallel;
    unsigned long long NumEndSerializedParallel;

    unsigned long long NumThreadInfo;
    unsigned long long NumGlobalThreadNum;
    unsigned long long NumGlobalNumThreads;
    unsigned long long NumBoundThreadNum;
    unsigned long long NumBoundNumThreads;
    unsigned long long NumInParallel;

    unsigned long long NumWorkSharing;
    unsigned long long NumMaster;
    unsigned long long NumEndMaster;
    unsigned long long NumOrdered;
    unsigned long long NumEndOrdered;
    unsigned long long NumCritical;
    unsigned long long NumEndCritical;
    unsigned long long NumSingle;
    unsigned long long NumEndSingle;
    unsigned long long NumForStaticFini;
    unsigned long long NumDispatchInit4;
    unsigned long long NumDispatchInit4u;
    unsigned long long NumDispatchInit8;
    unsigned long long NumDispatchInit8u;
    unsigned long long NumDistDispatchInit4;
    unsigned long long NumDispatchNext4;
    unsigned long long NumDispatchNext4u;
    unsigned long long NumDispatchNext8;
    unsigned long long NumDispatchNext8u;
    unsigned long long NumDispatchFini4;
    unsigned long long NumDispatchFini8;
    unsigned long long NumDispatchFini4u;
    unsigned long long NumDispatchFini8u;
    unsigned long long NumForStaticInit4;
    unsigned long long NumForStaticInit4u;
    unsigned long long NumForStaticInit8;
    unsigned long long NumForStaticInit8u;
    unsigned long long NumDistForStaticInit4;
    unsigned long long NumDistForStaticInit4u;
    unsigned long long NumDistForStaticInit8;
    unsigned long long NumDistForStaticInit8u;
    unsigned long long NumTeamStaticInit4;
    unsigned long long NumTeamStaticInit4u;
    unsigned long long NumTeamStaticInit8;
    unsigned long long NumTeamStaticInit8u;

    unsigned long long NumSynchronization;
    unsigned long long NumFlush;
    unsigned long long NumBarrier;
    unsigned long long NumBarrierMaster;
    unsigned long long NumEndBarrierMaster;
    unsigned long long NumBarrierMasterNowait;
    unsigned long long NumReduceNowait;
    unsigned long long NumEndReduceNowait;
    unsigned long long NumEndReduceWait;
    unsigned long long NumReduce;
    unsigned long long NumEndReduce;

    unsigned long long NumThreadPrivateDataSupport;
    unsigned long long NumCopyprivate;
    unsigned long long NumThreadprivateRegister;
    unsigned long long NumThreadprivateCached;
    unsigned long long NumThreadprivateRegisterVec;
    unsigned long long NumCtor;
    unsigned long long NumDtor;
    unsigned long long NumCctor;
    unsigned long long NumCtorVec;
    unsigned long long NumDtorVec;
    unsigned long long NumCctorVec;

    unsigned long long NumTaskingSupport;
    unsigned long long NumOmpTaskWithDeps;
    unsigned long long NumOmpWaitDeps;

    unsigned long long NumOthers;

    bool count_atomic(std::string name);
    bool count_startup_shutdown(std::string name);
    bool count_parallel(std::string name);
    bool count_thread_info(std::string name);
    bool count_synchronization(std::string name);
    bool count_work_sharing(std::string name);
    bool count_thread_private_data_support(std::string name);
    bool count_tasking_support(std::string name);

public:
    OpenMPstats(Module *M, int thread_id, int processor_id);
    void visit(Instruction &I);
    void JSONdump(JSONmanager *JSONwriter);
};

#endif // LLVM_OPEN_MP_STATS__H

