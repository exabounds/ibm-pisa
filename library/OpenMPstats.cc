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

#include "OpenMPstats.h"

OpenMPstats::OpenMPstats(Module *M, int thread_id, int processor_id) :
    InstructionAnalysis(M, thread_id, processor_id) {

    NumAtomic = 0;

    NumStartupShutdown = 0;
    NumBegin = 0;
    NumEnd = 0;

    NumParallel = 0;
    NumPushNumThreads = 0;
    NumForkCall = 0;
    NumPushNumTeams = 0;
    NumForkTeams = 0;
    NumSerializedParallel = 0;
    NumEndSerializedParallel = 0;

    NumThreadInfo = 0;
    NumGlobalThreadNum = 0;
    NumGlobalNumThreads = 0;
    NumBoundThreadNum = 0;
    NumBoundNumThreads = 0;
    NumInParallel = 0;

    NumWorkSharing = 0;
    NumMaster = 0;
    NumEndMaster = 0;
    NumOrdered = 0;
    NumEndOrdered = 0;
    NumCritical = 0;
    NumEndCritical = 0;
    NumSingle = 0;
    NumEndSingle = 0;
    NumForStaticFini = 0;
    NumDispatchInit4 = 0;
    NumDispatchInit4u =0;
    NumDispatchInit8 = 0;
    NumDispatchInit8u = 0;
    NumDistDispatchInit4 = 0;
    NumDispatchNext4 = 0;
    NumDispatchNext4u = 0;
    NumDispatchNext8 = 0;
    NumDispatchNext8u = 0;
    NumDispatchFini4 = 0;
    NumDispatchFini8 = 0;
    NumDispatchFini4u = 0;
    NumDispatchFini8u = 0;	
    NumForStaticInit4 = 0;
    NumForStaticInit4u = 0;
    NumForStaticInit8 = 0;
    NumForStaticInit8u = 0;
    NumDistForStaticInit4 = 0;
    NumDistForStaticInit4u = 0;
    NumDistForStaticInit8 = 0;
    NumDistForStaticInit8u = 0;
    NumTeamStaticInit4 = 0;
    NumTeamStaticInit4u = 0;
    NumTeamStaticInit8 = 0;
    NumTeamStaticInit8u = 0;

    NumSynchronization = 0;
    NumFlush = 0;
    NumBarrier = 0;
    NumBarrierMaster = 0;
    NumEndBarrierMaster = 0;
    NumBarrierMasterNowait = 0;
    NumReduceNowait = 0;
    NumEndReduceWait = 0;
    NumEndReduceNowait = 0;
    NumReduce = 0;
    NumEndReduce = 0;

    NumThreadPrivateDataSupport = 0;
    NumCopyprivate = 0;
    NumThreadprivateRegister = 0;
    NumThreadprivateCached = 0;
    NumThreadprivateRegisterVec = 0;
    NumCtor = 0;
    NumDtor = 0;
    NumCctor = 0;
    NumCtorVec = 0;
    NumDtorVec = 0;
    NumCctorVec = 0;

    NumTaskingSupport = 0;
    NumOmpTaskWithDeps = 0;
    NumOmpWaitDeps = 0;

    NumOthers = 0;
}


void OpenMPstats::JSONdump(JSONmanager *JSONwriter) {
    JSONwriter->String("openMPinstructionMix");
    JSONwriter->StartObject();

    JSONwriter->String("openMP_synchronization");
    JSONwriter->Uint64(NumSynchronization);

    JSONwriter->String("openMP_synchronization_flush");
    JSONwriter->Uint64(NumFlush);

    JSONwriter->String("openMP_synchronization_barrier");
    JSONwriter->Uint64(NumBarrier);

    JSONwriter->String("openMP_synchronization_barrier_master");
    JSONwriter->Uint64(NumBarrierMaster);

    JSONwriter->String("openMP_synchronization_end_barrier_master");
    JSONwriter->Uint64(NumEndBarrierMaster);

    JSONwriter->String("openMP_synchronization_barrier_master_nowait");
    JSONwriter->Uint64(NumBarrierMasterNowait);

    JSONwriter->String("openMP_synchronization_reduce_nowait");
    JSONwriter->Uint64(NumReduceNowait);

    JSONwriter->String("openMP_synchronization_end_reduce_wait");
    JSONwriter->Uint64(NumEndReduceWait);

    JSONwriter->String("openMP_synchronization_reduce");
    JSONwriter->Uint64(NumReduce);

    JSONwriter->String("openMP_synchronization_end_reduce");
    JSONwriter->Uint64(NumEndReduce);


    JSONwriter->String("openMP_atomic");
    JSONwriter->Uint64(NumAtomic);


    JSONwriter->String("openMP_startup_shutdown");
        JSONwriter->Uint64(NumStartupShutdown);

        JSONwriter->String("openMP_startup_shutdown_begin");
        JSONwriter->Uint64(NumBegin);

        JSONwriter->String("openMP_startup_shutdown_end");
        JSONwriter->Uint64(NumEnd);


        JSONwriter->String("openMP_parallel");
        JSONwriter->Uint64(NumParallel);

        JSONwriter->String("openMP_parallel_push_num_threads");
        JSONwriter->Uint64(NumPushNumThreads);

        JSONwriter->String("openMP_parallel_fork_call");
        JSONwriter->Uint64(NumForkCall);

        JSONwriter->String("openMP_parallel_push_num_teams");
        JSONwriter->Uint64(NumPushNumTeams);

        JSONwriter->String("openMP_parallel_fork_teams");
        JSONwriter->Uint64(NumForkTeams);

        JSONwriter->String("openMP_parallel_serialized_parallel");
        JSONwriter->Uint64(NumSerializedParallel);

        JSONwriter->String("openMP_parallel_end_serialized_parallel");
        JSONwriter->Uint64(NumEndSerializedParallel);


        JSONwriter->String("openMP_thread_info");
        JSONwriter->Uint64(NumThreadInfo);

        JSONwriter->String("openMP_thread_info_global_thread_num");
        JSONwriter->Uint64(NumGlobalThreadNum);

        JSONwriter->String("openMP_thread_info_global_num_threads");
        JSONwriter->Uint64(NumGlobalNumThreads);

        JSONwriter->String("openMP_thread_info_bound_thread_num");
        JSONwriter->Uint64(NumBoundThreadNum);

        JSONwriter->String("openMP_thread_info_bound_num_threads");
        JSONwriter->Uint64(NumBoundNumThreads);

        JSONwriter->String("openMP_thread_info_in_parallel");
        JSONwriter->Uint64(NumInParallel);


        JSONwriter->String("openMP_work_sharing");
        JSONwriter->Uint64(NumWorkSharing);

        JSONwriter->String("openMP_work_sharing_master");
        JSONwriter->Uint64(NumMaster);

        JSONwriter->String("openMP_work_sharing_end_master");
        JSONwriter->Uint64(NumEndMaster);

        JSONwriter->String("openMP_work_sharing_ordered");
        JSONwriter->Uint64(NumOrdered);

        JSONwriter->String("openMP_work_sharing_end_ordered");
        JSONwriter->Uint64(NumEndOrdered);

        JSONwriter->String("openMP_work_sharing_critical");
        JSONwriter->Uint64(NumCritical);

        JSONwriter->String("openMP_work_sharing_end_critical");
        JSONwriter->Uint64(NumEndCritical);

        JSONwriter->String("openMP_work_sharing_single");
        JSONwriter->Uint64(NumSingle);

        JSONwriter->String("openMP_work_sharing_end_single");
        JSONwriter->Uint64(NumEndSingle);

        JSONwriter->String("openMP_work_sharing_for_static_fini");
        JSONwriter->Uint64(NumForStaticFini);

        JSONwriter->String("openMP_work_sharing_dispatch_init_4");
        JSONwriter->Uint64(NumDispatchInit4);

        JSONwriter->String("openMP_work_sharing_dispatch_init_4u");
        JSONwriter->Uint64(NumDispatchInit4u);

        JSONwriter->String("openMP_work_sharing_dispatch_init_8");
        JSONwriter->Uint64(NumDispatchInit8);

        JSONwriter->String("openMP_work_sharing_dispatch_init_8u");
        JSONwriter->Uint64(NumDispatchInit8u);

        JSONwriter->String("openMP_work_sharing_dist_dispatch_init_4");
        JSONwriter->Uint64(NumDistDispatchInit4);

        JSONwriter->String("openMP_work_sharing_dispatch_next_4");
        JSONwriter->Uint64(NumDispatchNext4);

        JSONwriter->String("openMP_work_sharing_dispatch_next_4u");
        JSONwriter->Uint64(NumDispatchNext4u);

        JSONwriter->String("openMP_work_sharing_dispatch_next_8");
        JSONwriter->Uint64(NumDispatchNext8);

        JSONwriter->String("openMP_work_sharing_dispatch_next_8u");
        JSONwriter->Uint64(NumDispatchNext8u);

        JSONwriter->String("openMP_work_sharing_dispatch_fini_4");
        JSONwriter->Uint64(NumDispatchFini4);

    JSONwriter->String("openMP_work_sharing_dispatch_fini_4u");
        JSONwriter->Uint64(NumDispatchFini4u);

        JSONwriter->String("openMP_work_sharing_dispatch_fini_8");
        JSONwriter->Uint64(NumDispatchFini8);

        JSONwriter->String("openMP_work_sharing_dispatch_fini_8u");
        JSONwriter->Uint64(NumDispatchFini8u);


        JSONwriter->String("openMP_work_sharing_for_static_init_4");
        JSONwriter->Uint64(NumForStaticInit4);

        JSONwriter->String("openMP_work_sharing_for_static_init_4u");
        JSONwriter->Uint64(NumForStaticInit4u);

        JSONwriter->String("openMP_work_sharing_for_static_init_8");
        JSONwriter->Uint64(NumForStaticInit8);

        JSONwriter->String("openMP_work_sharing_for_static_init_8u");
        JSONwriter->Uint64(NumForStaticInit8u);

        JSONwriter->String("openMP_work_sharing_dist_for_static_init_4");
        JSONwriter->Uint64(NumDistForStaticInit4);

        JSONwriter->String("openMP_work_sharing_dist_for_static_init_4u");
        JSONwriter->Uint64(NumDistForStaticInit4u);

        JSONwriter->String("openMP_work_sharing_dist_for_static_init_8");
        JSONwriter->Uint64(NumDistForStaticInit8);

        JSONwriter->String("openMP_work_sharing_dist_for_static_init_8u");
        JSONwriter->Uint64(NumDistForStaticInit8u);

        JSONwriter->String("openMP_work_sharing_team_static_init_4");
        JSONwriter->Uint64(NumTeamStaticInit4);

        JSONwriter->String("openMP_work_sharing_team_static_init_4u");
        JSONwriter->Uint64(NumTeamStaticInit4u);

        JSONwriter->String("openMP_work_sharing_team_static_init_8");
        JSONwriter->Uint64(NumTeamStaticInit8);

        JSONwriter->String("openMP_work_sharing_team_static_init_8u");
        JSONwriter->Uint64(NumTeamStaticInit8u);


        JSONwriter->String("openMP_thread_private_data_support");
        JSONwriter->Uint64(NumThreadPrivateDataSupport);

        JSONwriter->String("openMP_thread_private_data_support_copy_private");
        JSONwriter->Uint64(NumCopyprivate);

        JSONwriter->String("openMP_thread_private_data_support_thread_private_register");
        JSONwriter->Uint64(NumThreadprivateRegister);

        JSONwriter->String("openMP_thread_private_data_support_thread_pivate_cached");
        JSONwriter->Uint64(NumThreadprivateCached);

        JSONwriter->String("openMP_thread_private_data_support_thread_private_register_vec");
        JSONwriter->Uint64(NumThreadprivateRegisterVec);

        JSONwriter->String("openMP_thread_private_data_support_ctor");
        JSONwriter->Uint64(NumCtor);

        JSONwriter->String("openMP_thread_private_data_support_dtor");
        JSONwriter->Uint64(NumDtor);

        JSONwriter->String("openMP_thread_private_data_support_cctor");
        JSONwriter->Uint64(NumCctor);

        JSONwriter->String("openMP_thread_private_data_support_cctor_vec");
        JSONwriter->Uint64(NumCctorVec);

        JSONwriter->String("openMP_thread_private_data_support_dtor_vec");
        JSONwriter->Uint64(NumDtorVec);

        JSONwriter->String("openMP_thread_private_data_support_cctor_vec");
        JSONwriter->Uint64(NumCctorVec);


        JSONwriter->String("openMP_tasking_support");
        JSONwriter->Uint64(NumTaskingSupport);

        JSONwriter->String("openMP_tasking_support_omp_task_with_deps");
        JSONwriter->Uint64(NumOmpTaskWithDeps);

        JSONwriter->String("openMP_tasking_support_omp_wait_deps");
        JSONwriter->Uint64(NumOmpWaitDeps);


        JSONwriter->String("openMP_others");
        JSONwriter->Uint64(NumOthers);


    JSONwriter->EndObject();
}

void OpenMPstats::visit(Instruction &I) {
    if (!strcmp(I.getOpcodeName(), "call")) {
        CallInst *call = cast<CallInst>(&I);
        Function *_call = get_calledFunction(call);

        if (_call) {
            std::size_t found = _call->getName().str().find("kmpc");

            if (found != std::string::npos) {
                if (count_atomic(_call->getName().str()))
                    return;

                if (count_startup_shutdown(_call->getName().str()))
                        return;

                if (count_parallel(_call->getName().str()))
                        return;

                if (count_thread_info(_call->getName().str()))
                        return;

                if (count_work_sharing(_call->getName().str()))
                        return;

                if (count_synchronization(_call->getName().str()))
                        return;

                if (count_thread_private_data_support(_call->getName().str()))
                        return;

                if (count_tasking_support(_call->getName().str()))
                        return;

                NumOthers++;
            }
        }
    }
    
    return;
}

bool OpenMPstats::count_atomic(std::string name) {
    std::size_t found;
    found = name.find("atomic");
    
    if (found != std::string::npos) {
        NumAtomic++;
        return true;
    }

    return false;
}

bool OpenMPstats::count_startup_shutdown(std::string name) {
    if (name == "__kmpc_begin") {
        NumBegin++;
        NumStartupShutdown++;
        return true;
    }

    if (name == "__kmpc_end") {
        NumEnd++;
        NumStartupShutdown++;
        return true;
    }

    return false;
}

bool OpenMPstats::count_parallel(std::string name) {
    if (name == "__kmpc_push_num_threads") {
        NumPushNumThreads++;
        NumParallel++;
        return true;
    }

    if (name == "__kmpc_fork_call") {
        NumForkCall++;
        NumParallel++;
        return true;
    }

    if (name == "__kmpc_push_num_teams") {
        NumPushNumTeams++;
        NumParallel++;
        return true;
    }

    if (name == "__kmpc_fork_teams") {
        NumForkTeams++;
        NumParallel++;
        return true;
    }

    if (name == "__kmpc_serialized_parallel") {
        NumSerializedParallel++;
        NumParallel++;
        return true;
    }

    if (name == "__kmpc_end_serialized_parallel") {
        NumEndSerializedParallel++;
        NumParallel++;
        return true;
    }

    return false;
}


bool OpenMPstats::count_thread_info(std::string name) {
    if (name == "__kmpc_global_thread_num") {
        NumGlobalThreadNum++;
        NumThreadInfo++;
        return true;
    }

    if (name == "__kmpc_global_num_threads") {
        NumGlobalNumThreads++;
        NumThreadInfo++;
        return true;
    }

    if (name == "__kmpc_bound_thread_num") {
        NumBoundThreadNum++;
        NumThreadInfo++;
        return true;
    }

    if (name == "__kmpc_bound_num_threads") {
        NumBoundNumThreads++;
        NumThreadInfo++;
        return true;
    }

    if (name == "__kmpc_in_parallel") {
        NumInParallel++;
        NumThreadInfo++;
        return true;
    }

    return false;
}

bool OpenMPstats::count_work_sharing(std::string name) {
    if (name == "__kmpc_master") {
        NumMaster++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_end_master") {
        NumEndMaster++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_ordered") {
        NumOrdered++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_end_ordered") {
        NumEndOrdered++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_critical") {
        NumCritical++;
        NumWorkSharing++;
        return true;
    }


    if (name == "__kmpc_end_critical") {
        NumEndCritical++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_single") {
        NumSingle++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_end_single") {
        NumEndSingle++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_for_static_fini") {
        NumForStaticFini++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_init_4") {
        NumDispatchInit4++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_init_4u") {
        NumDispatchInit4u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_init_8") {
        NumDispatchInit8++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_init_8u") {
        NumDispatchInit8u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dist_dispatch_init_4") {
        NumDistDispatchInit4++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_next_4") {
        NumDispatchNext4++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_next_4u") {
        NumDispatchNext4u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_next_8") {
        NumDispatchNext8++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_next_8u") {
        NumDispatchNext8u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_fini_4") {
        NumDispatchFini4++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_fini_4u") {
        NumDispatchFini4u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_fini_8") {
        NumDispatchFini8++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dispatch_fini_8u") {
        NumDispatchFini8u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_for_static_init_4") {
        NumForStaticInit4++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_for_static_init_4u") {
        NumForStaticInit4u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_for_static_init_8") {
        NumForStaticInit8++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_for_static_init_8u") {
        NumForStaticInit8u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dist_for_static_init_4") {
        NumDistForStaticInit4++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dist_for_static_init_4u") {
        NumDistForStaticInit4u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dist_for_static_init_8") {
        NumDistForStaticInit8++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_dist_for_static_init_8u") {
        NumDistForStaticInit8u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_team_static_init_4") {
        NumTeamStaticInit4++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_team_static_init_4u") {
        NumTeamStaticInit4u++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_team_static_init_8") {
        NumTeamStaticInit8++;
        NumWorkSharing++;
        return true;
    }

    if (name == "__kmpc_team_static_init_8u") {
        NumTeamStaticInit8u++;
        NumWorkSharing++;
        return true;
    }

    return false;
}

bool OpenMPstats::count_synchronization(std::string name) {
    if (name == "__kmpc_flush") {
        NumFlush++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_barrier") {
        NumBarrier++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_barrier_master") {
        NumBarrierMaster++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_end_barrier_master") {
        NumEndBarrierMaster++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_barrier_master_nowait") {
        NumBarrierMasterNowait++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_reduce_nowait") {
        NumReduceNowait++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_end_reduce_nowait") {
        NumEndReduceNowait++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_reduce") {
        NumReduce++;
        NumSynchronization++;
        return true;
    }

    if (name == "__kmpc_end_reduce") {
        NumEndReduce++;
        NumSynchronization++;
        return true;
    }

    return false;

}

bool OpenMPstats::count_thread_private_data_support(std::string name) {
    if (name == "__kmpc_copyprivate") {
        NumCopyprivate++;
        NumThreadPrivateDataSupport++;
        return true;
        }

    if (name == "__kmpc_threadprivate_register") {
        NumThreadprivateRegister++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "__kmpc_threadprivate_cached") {
        NumThreadprivateCached++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "__kmpc_threadprivate_register_vec") {
        NumThreadprivateRegisterVec++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "kmpc_ctor") {
        NumCtor++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "kmpc_dtor") {
        NumDtor++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "kmpc_cctor") {
        NumCctor++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "kmpc_ctor_vec") {
        NumCtorVec++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "kmpc_dtor_vec") {
        NumDtorVec++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    if (name == "kmpc_cctor_vec") {
        NumCctorVec++;
        NumThreadPrivateDataSupport++;
        return true;
    }

    return false;
}

bool OpenMPstats::count_tasking_support(std::string name) {
    if (name == "__kmpc_omp_task_with_deps") {
        NumOmpTaskWithDeps++;
        NumTaskingSupport++;
        return true;
    }

    if (name == "__kmpc_omp_wait_deps") {
        NumOmpWaitDeps++;
        NumTaskingSupport++;
        return true;
    }

    return false;
}
