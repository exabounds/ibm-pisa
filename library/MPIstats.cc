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

#include "MPIstats.h"

MPIstats::MPIstats(Module *M, int thread_id, int processor_id) :
    InstructionAnalysis(M, thread_id, processor_id) {
    NumMPIEnvironmentManagement = 0;
    NumMPIPointToPointCommunication = 0;
    NumMPICollectiveCommunication = 0;
    NumMPIProcessGroup = 0;
    NumMPICommunicators = 0;
    NumMPIDerivedTypes = 0;
    NumMPIVirtualTopology = 0;
    NumMPIMiscellaneous = 0;
    NumMPIOthers = 0;


    NumMPIBsend = 0;
    NumMPIBsend_init = 0;
    NumMPIBuffer_attach = 0;
    NumMPIBuffer_detach = 0;
    NumMPICancel = 0;
    NumMPIGet_count = 0;
    NumMPIGet_elements = 0;
    NumMPIIbsend = 0;
    NumMPIIprobe = 0;
    NumMPIIrecv = 0;
    NumMPIIrsend = 0;
    NumMPIIsend = 0;
    NumMPIIssend = 0;
    NumMPIProbe = 0;
    NumMPIRecv = 0;
    NumMPIRecv_init = 0;
    NumMPIRequest_free = 0;
    NumMPIRsend = 0;
    NumMPIRsend_init = 0;
    NumMPISend = 0;
    NumMPISend_init = 0;
    NumMPISendrecv = 0;
    NumMPISendrecv_replace = 0;
    NumMPISsend = 0;
    NumMPISsend_init = 0;
    NumMPIStart = 0;
    NumMPIStartall = 0;
    NumMPITest = 0;
    NumMPITest_cancelled = 0;
    NumMPITestall = 0;
    NumMPITestany = 0;
    NumMPITestsome = 0;
    NumMPIWait = 0;
    NumMPIWaitall = 0;
    NumMPIWaitany = 0;
    NumMPIWaitsome = 0;

    NumMPIAllgather = 0;
    NumMPIAllgatherv = 0;
    NumMPIAllreduce = 0;
    NumMPIAlltoall = 0;
    NumMPIAlltoallv = 0;
    NumMPIBarrier = 0;
    NumMPIBcast = 0;
    NumMPIGather = 0;
    NumMPIGatherv = 0;
    NumMPIOp_create = 0;
    NumMPIOp_free = 0;
    NumMPIReduce = 0;
    NumMPIReduce_scatter = 0;
    NumMPIScan = 0;
    NumMPIScatter = 0;
    NumMPIScatterv = 0;
}

void MPIstats::JSONdump(JSONmanager *JSONwriter) {
    JSONwriter->String("mpiInstructionMix");
    JSONwriter->StartArray();
    JSONwriter->StartObject();

    JSONwriter->String("MPI_instructions_analyzed");
    JSONwriter->Uint64(NumMPIEnvironmentManagement + NumMPIPointToPointCommunication + 
                        NumMPICollectiveCommunication + NumMPIProcessGroup +
                        NumMPICommunicators + NumMPIDerivedTypes + 
                        NumMPIVirtualTopology + NumMPIMiscellaneous + NumMPIOthers);

    JSONwriter->String("environmentManagementRoutines");
    JSONwriter->StartObject();
        JSONwriter->String("MPI_environment_management_calls");
        JSONwriter->Uint64(NumMPIEnvironmentManagement);
    JSONwriter->EndObject();

    JSONwriter->String("p2pCommunicationRoutines");
    JSONwriter->StartObject();

        JSONwriter->String("MPI_Bsend");
        JSONwriter->Uint64(NumMPIBsend);

        JSONwriter->String("MPI_Bsend_init");
        JSONwriter->Uint64(NumMPIBsend_init);

        JSONwriter->String("MPI_Buffer_attach");
        JSONwriter->Uint64(NumMPIBuffer_attach);

        JSONwriter->String("MPI_Bsend_detach");
        JSONwriter->Uint64(NumMPIBuffer_detach);

        JSONwriter->String("MPI_Cancel");
        JSONwriter->Uint64(NumMPICancel);

        JSONwriter->String("MPI_Get_count");
        JSONwriter->Uint64(NumMPIGet_count);

        JSONwriter->String("MPI_Get_elements");
        JSONwriter->Uint64(NumMPIGet_elements);

        JSONwriter->String("MPI_Ibsend");
        JSONwriter->Uint64(NumMPIIbsend);

        JSONwriter->String("MPI_Iprobe");
        JSONwriter->Uint64(NumMPIIprobe);

        JSONwriter->String("MPI_Irecv");
        JSONwriter->Uint64(NumMPIIrecv);

        JSONwriter->String("MPI_Irsend");
        JSONwriter->Uint64(NumMPIIrsend);

        JSONwriter->String("MPI_Isend");
        JSONwriter->Uint64(NumMPIIsend);

        JSONwriter->String("MPI_Issend");
        JSONwriter->Uint64(NumMPIIssend);

        JSONwriter->String("MPI_Probe");
        JSONwriter->Uint64(NumMPIProbe);

        JSONwriter->String("MPI_Recv");
        JSONwriter->Uint64(NumMPIRecv);

        JSONwriter->String("MPI_Recv_init");
        JSONwriter->Uint64(NumMPIRecv_init);

        JSONwriter->String("MPI_Request_free");
        JSONwriter->Uint64(NumMPIRequest_free);

        JSONwriter->String("MPI_Rsend");
        JSONwriter->Uint64(NumMPIRsend);

        JSONwriter->String("MPI_Rsend_init");
        JSONwriter->Uint64(NumMPIRsend_init);

        JSONwriter->String("MPI_Send");
        JSONwriter->Uint64(NumMPISend);

        JSONwriter->String("MPI_Send_init");
        JSONwriter->Uint64(NumMPISend_init);

        JSONwriter->String("MPI_Sendrecv");
        JSONwriter->Uint64(NumMPISendrecv);

        JSONwriter->String("MPI_Sendrecv_replace");
        JSONwriter->Uint64(NumMPISendrecv_replace);

        JSONwriter->String("MPI_Ssend");
        JSONwriter->Uint64(NumMPISsend);

        JSONwriter->String("MPI_Ssend_init");
        JSONwriter->Uint64(NumMPISsend_init);

        JSONwriter->String("MPI_Start");
        JSONwriter->Uint64(NumMPIStart);

        JSONwriter->String("MPI_Startall");
        JSONwriter->Uint64(NumMPIStartall);

        JSONwriter->String("MPI_Test");
        JSONwriter->Uint64(NumMPITest);

        JSONwriter->String("MPI_Test_cancelled");
        JSONwriter->Uint64(NumMPITest_cancelled);

        JSONwriter->String("MPI_Testall");
        JSONwriter->Uint64(NumMPITestall);

        JSONwriter->String("MPI_Testany");
        JSONwriter->Uint64(NumMPITestany);

        JSONwriter->String("MPI_Testsome");
        JSONwriter->Uint64(NumMPITestsome);

        JSONwriter->String("MPI_Wait");
        JSONwriter->Uint64(NumMPIWait);

        JSONwriter->String("MPI_Waitall");
        JSONwriter->Uint64(NumMPIWaitall);

        JSONwriter->String("MPI_Waitany");
        JSONwriter->Uint64(NumMPIWaitany);

        JSONwriter->String("MPI_Waitsome");
        JSONwriter->Uint64(NumMPIWaitsome);

    JSONwriter->EndObject();

    JSONwriter->String("collectiveCommunicationRoutines");
    JSONwriter->StartObject();

        JSONwriter->String("MPI_Allgather");
        JSONwriter->Uint64(NumMPIAllgather);

        JSONwriter->String("MPI_Allgatherv");
        JSONwriter->Uint64(NumMPIAllgatherv);

        JSONwriter->String("MPI_Allreduce");
        JSONwriter->Uint64(NumMPIAllreduce);

        JSONwriter->String("MPI_Alltoall");
        JSONwriter->Uint64(NumMPIAlltoall);

        JSONwriter->String("MPI_Alltoallv");
        JSONwriter->Uint64(NumMPIAlltoallv);

        JSONwriter->String("MPI_Barrier");
        JSONwriter->Uint64(NumMPIBarrier);

        JSONwriter->String("MPI_Bcast");
        JSONwriter->Uint64(NumMPIBcast);

        JSONwriter->String("MPI_Gather");
        JSONwriter->Uint64(NumMPIGather);

        JSONwriter->String("MPI_Gatherv");
        JSONwriter->Uint64(NumMPIGatherv);

        JSONwriter->String("MPI_Op_create");
        JSONwriter->Uint64(NumMPIOp_create);

        JSONwriter->String("MPI_Op_free");
        JSONwriter->Uint64(NumMPIOp_free);

        JSONwriter->String("MPI_Reduce");
        JSONwriter->Uint64(NumMPIReduce);

        JSONwriter->String("MPI_Reduce_Scatter");
        JSONwriter->Uint64(NumMPIReduce_scatter);

        JSONwriter->String("MPI_Scan");
        JSONwriter->Uint64(NumMPIScan);

        JSONwriter->String("MPI_Scatter");
        JSONwriter->Uint64(NumMPIScatter);

        JSONwriter->String("MPI_Scatterv");
        JSONwriter->Uint64(NumMPIScatterv);

    JSONwriter->EndObject();

    JSONwriter->String("processGroupRoutines");
    JSONwriter->StartObject();
        JSONwriter->String("MPI_Group_calls");
        JSONwriter->Uint64(NumMPIProcessGroup);
    JSONwriter->EndObject();

    JSONwriter->String("communicatorsRoutines");
    JSONwriter->StartObject();
        JSONwriter->String("MPI_Comm_calls");
        JSONwriter->Uint64(NumMPICommunicators);
    JSONwriter->EndObject();

    JSONwriter->String("derivedTypesRoutines");
    JSONwriter->StartObject();
        JSONwriter->String("MPI_Type_calls");
        JSONwriter->Uint64(NumMPIDerivedTypes);
    JSONwriter->EndObject();

    JSONwriter->String("virtualTopologyRoutines");
    JSONwriter->StartObject();
        JSONwriter->String("MPI_virtual_topology_calls");
        JSONwriter->Uint64(NumMPIVirtualTopology);
    JSONwriter->EndObject();

    JSONwriter->String("miscellaneousRoutines");
    JSONwriter->StartObject();
        JSONwriter->String("MPI_misc_calls");
        JSONwriter->Uint64(NumMPIMiscellaneous);
    JSONwriter->EndObject();

    JSONwriter->String("otherRoutines");
    JSONwriter->StartObject();
        JSONwriter->String("MPI_other_calls");
        JSONwriter->Uint64(NumMPIOthers);
    JSONwriter->EndObject();

    JSONwriter->EndObject();
    JSONwriter->EndArray();

}

void MPIstats::visit(Instruction &I) {
    if (!strcmp(I.getOpcodeName(), "call")) {
        CallInst *call = cast<CallInst>(&I);
        Function *_call = get_calledFunction(call);
        

        if (_call && !strncasecmp(_call->getName().str().c_str(), "MPI_", 4)) {
            
            if (count_environment_management(_call->getName().str()))
                return;

            if (count_point_to_point_communication(_call->getName().str()))
                return;

            if (count_collective_communication(_call->getName().str()))
                return;

            if (count_process_group(_call->getName().str()))
                return;

            if (count_communicators(_call->getName().str()))
                return;

            if (count_derived_types(_call->getName().str()))
                return;

            if (count_virtual_topology(_call->getName().str()))
                return;

            if (count_miscellaneous(_call->getName().str()))
                return;

            NumMPIOthers++;
        }
    }
}

bool MPIstats::count_environment_management(std::string name) {
    if(name.back() == '_' )
        name.pop_back();

    const char* cname = name.c_str();
    if (!strcasecmp(cname, "MPI_abort") || !strcasecmp(cname, "MPI_Errhandler_create") ||
        !strcasecmp(cname, "MPI_Errhandler_free") || !strcasecmp(cname, "MPI_Errhandler_get") ||
        !strcasecmp(cname, "MPI_Errhandler_set") || !strcasecmp(cname, "MPI_Error_class") ||
        !strcasecmp(cname, "MPI_Error_string") || !strcasecmp(cname, "MPI_Finalize") ||
        !strcasecmp(cname, "MPI_Get_processor_name") || !strcasecmp(cname, "MPI_Get_version") ||
        !strcasecmp(cname, "MPI_Init") || !strcasecmp(cname, "MPI_Initialized") ||
        !strcasecmp(cname, "MPI_Wtick") || !strcasecmp(cname, "MPI_Wtime")) {
        NumMPIEnvironmentManagement++;
        return true;
    }

    return false;
}

bool MPIstats::count_point_to_point_communication(std::string name) {
    if(name.back() == '_' )
        name.pop_back();

    const char* cname = name.c_str();
    bool found = false;

    if (!strcasecmp(cname, "MPI_Bsend")) {
        found = true;
        NumMPIBsend++;
    } else if (!strcasecmp(cname, "MPI_Bsend_init")) {
        found = true;
        NumMPIBsend_init++;
    } else if (!strcasecmp(cname, "MPI_Buffer_attach")) {
        found = true;
        NumMPIBuffer_attach++;
    } else if (!strcasecmp(cname, "MPI_Buffer_detach")) {
        found = true;
        NumMPIBuffer_detach++;
    } else if (!strcasecmp(cname, "MPI_Cancel")) {
        found = true;
        NumMPICancel++;
    } else if (!strcasecmp(cname, "MPI_Get_count")) {
        found = true;
        NumMPIGet_count++;
    } else if (!strcasecmp(cname, "MPI_Get_elements")) {
        found = true;
        NumMPIGet_elements++;
    } else if (!strcasecmp(cname, "MPI_Ibsend")) {
        found = true;
        NumMPIIbsend++;
    } else if (!strcasecmp(cname, "MPI_Iprobe")) {
        found = true;
        NumMPIIprobe++;
    } else if (!strcasecmp(cname, "MPI_Irecv")) {
        found = true;
        NumMPIIrecv++;
    } else if (!strcasecmp(cname, "MPI_Irsend")) {
        found = true;
        NumMPIIrsend++;
    } else if (!strcasecmp(cname, "MPI_Isend")) {
        found = true;
        NumMPIIsend++;
    } else if (!strcasecmp(cname, "MPI_Issend")) {
        found = true;
        NumMPIIssend++;
    } else if (!strcasecmp(cname, "MPI_Probe")) {
        found = true;
        NumMPIProbe++;
    } else if (!strcasecmp(cname, "MPI_Recv")) {
        found = true;
        NumMPIRecv++;
    } else if (!strcasecmp(cname, "MPI_Recv_init")) {
        found = true;
        NumMPIRecv_init++;
    } else if (!strcasecmp(cname, "MPI_Request_free")) {
        found = true;
        NumMPIRequest_free++;
    } else if (!strcasecmp(cname, "MPI_Rsend")) {
        found = true;
        NumMPIRsend++;
    } else if (!strcasecmp(cname, "MPI_Rsend_init")) {
        found = true;
        NumMPIRsend_init++;
    } else if (!strcasecmp(cname, "MPI_Send")) {
        found = true;
        NumMPISend++;
    } else if (!strcasecmp(cname, "MPI_Send_init")) {
        found = true;
        NumMPISend_init++;
    } else if (!strcasecmp(cname, "MPI_Sendrecv")) {
        found = true;
        NumMPISendrecv++;
    } else if (!strcasecmp(cname, "MPI_Sendrecv_replace")) {
        found = true;
        NumMPISendrecv_replace++;
    } else if (!strcasecmp(cname, "MPI_Ssend")) {
        found = true;
        NumMPISsend++;
    } else if (!strcasecmp(cname, "MPI_Ssend_init")) {
        found = true;
        NumMPISsend_init++;
    } else if (!strcasecmp(cname, "MPI_Start")) {
        found = true;
        NumMPIStart++;
    } else if (!strcasecmp(cname, "MPI_Startall")) {
        found = true;
        NumMPIStartall++;
    } else if (!strcasecmp(cname, "MPI_Test")) {
        found = true;
        NumMPITest++;
    } else if (!strcasecmp(cname, "MPI_Test_cancelled")) {
        found = true;
        NumMPITest_cancelled++;
    } else if (!strcasecmp(cname, "MPI_Testall")) {
        found = true;
        NumMPITestall++;
    } else if (!strcasecmp(cname, "MPI_Testany")) {
        found = true;
        NumMPITestany++;
    } else if (!strcasecmp(cname, "MPI_Testsome")) {
        found = true;
        NumMPITestsome++;
    } else if (!strcasecmp(cname, "MPI_Wait")) {
        found = true;
        NumMPIWait++;
    } else if (!strcasecmp(cname, "MPI_Waitall")) {
        found = true;
        NumMPIWaitall++;
    } else if (!strcasecmp(cname, "MPI_Waitany")) {
        found = true;
        NumMPIWaitany++;
    } else if (!strcasecmp(cname, "MPI_Waitsome")) {
        found = true;
        NumMPIWaitsome++;
    }

    if (found)
        NumMPIPointToPointCommunication++;

    return found;
}

bool MPIstats::count_collective_communication(std::string name) {
    if(name.back() == '_' )
        name.pop_back();

    const char* cname = name.c_str();
    bool found = false;

    if (!strcasecmp(cname, "MPI_Allgather")) {
        found = true;
        NumMPIAllgather++;
    } else if (!strcasecmp(cname, "MPI_Allgatherv")) {
        found = true;
        NumMPIAllgatherv++;
    } else if (!strcasecmp(cname, "MPI_Allreduce")) {
        found = true;
        NumMPIAllreduce++;
    } else if (!strcasecmp(cname, "MPI_Alltoall")) {
        found = true;
        NumMPIAlltoall++;
    } else if (!strcasecmp(cname, "MPI_Alltoallv")) {
        found = true;
        NumMPIAlltoallv++;
    } else if (!strcasecmp(cname, "MPI_Barrier")) {
        found = true;
        NumMPIBarrier++;
    } else if (!strcasecmp(cname, "MPI_Bcast")) {
        found = true;
        NumMPIBcast++;
    } else if (!strcasecmp(cname, "MPI_Gather")) {
        found = true;
        NumMPIGather++;
    } else if (!strcasecmp(cname, "MPI_Gatherv")) {
        found = true;
        NumMPIGatherv++;
    } else if (!strcasecmp(cname, "MPI_Op_create")) {
        found = true;
        NumMPIOp_create++;
    } else if (!strcasecmp(cname, "MPI_Op_free")) {
        found = true;
        NumMPIOp_free++;
    } else if (!strcasecmp(cname, "MPI_Reduce")) {
        found = true;
        NumMPIReduce++;
    } else if (!strcasecmp(cname, "MPI_Reduce_scatter")) {
        found = true;
        NumMPIReduce_scatter++;
    } else if (!strcasecmp(cname, "MPI_Scan")) {
        found = true;
        NumMPIScan++;
    } else if (!strcasecmp(cname, "MPI_Scatter")) {
        found = true;
        NumMPIScatter++;
    } else if (!strcasecmp(cname, "MPI_Scatterv")) {
        found = true;
        NumMPIScatterv++;
    }

    if (found)
        NumMPICollectiveCommunication++;

    return found;
}

bool MPIstats::count_process_group(std::string name) {
    if(name.back() == '_' )
        name.pop_back();

    if (!strncasecmp(name.c_str(), "MPI_Group_", 10)) {
        NumMPIProcessGroup++;
        return true;
    }

    return false;
}

bool MPIstats::count_communicators(std::string name) {
    if (!strncasecmp(name.c_str(), "MPI_Comm_", 9)) {
        NumMPICommunicators++;
        return true;
    }

    if (!strncasecmp(name.c_str(), "MPI_Intercomm_", 14)) {
        NumMPICommunicators++;
        return true;
    }

    return false;
}

bool MPIstats::count_derived_types(std::string name) {
    if (!strncasecmp(name.c_str(), "MPI_Type_", 9)) {
        NumMPIDerivedTypes++;
        return true;
    }

    return false;
}

bool MPIstats::count_virtual_topology(std::string name) {
    if(name.back() == '_' )
        name.pop_back();

    const char* cname = name.c_str();
    if (!strncasecmp(name.c_str(), "MPI_Cart_", 9)) {
        NumMPIVirtualTopology++;
        return true;
    }

    if (!strncasecmp(name.c_str(), "MPI_Graph_", 10)) {
        NumMPIVirtualTopology++;
        return true;
    }

    if (!strcasecmp(cname, "MPI_Cartdim_get") || !strcasecmp(cname, "MPI_Dims_create") ||
        !strcasecmp(cname, "MPI_Graphdims_get") || !strcasecmp(cname, "MPI_Topo_create")) {
        NumMPIVirtualTopology++;
        return true;
    }

    return false;
}

bool MPIstats::count_miscellaneous(std::string name) {
    if(name.back() == '_' )
        name.pop_back();

    const char* cname = name.c_str();
    if (!strcasecmp(cname, "MPI_Address") || !strcasecmp(cname, "MPI_Attr_delete") || !strcasecmp(cname, "MPI_Attr_get") ||
        !strcasecmp(cname, "MPI_Attr_put") || !strcasecmp(cname, "MPI_Keyval_create") || !strcasecmp(cname, "MPI_Keyval_free") ||
        !strcasecmp(cname, "MPI_Pack") || !strcasecmp(cname, "MPI_Pack_size") || !strcasecmp(cname, "MPI_Pcontrol") ||
        !strcasecmp(cname, "MPI_Unpack")) {
        NumMPIMiscellaneous++;
        return true;
    }

    return false;
}
