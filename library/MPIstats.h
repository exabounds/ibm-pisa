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

class MPIstats;

#ifndef LLVM_MPI_STATS__H
#define LLVM_MPI_STATS__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "JSONmanager.h"
#include "splay.h"
#include "MPIcnfSupport.h"

class MPIstats: public InstructionAnalysis {
    unsigned long long NumMPIEnvironmentManagement;
    unsigned long long NumMPIPointToPointCommunication;
    unsigned long long NumMPICollectiveCommunication;
    unsigned long long NumMPIProcessGroup;
    unsigned long long NumMPICommunicators;
    unsigned long long NumMPIDerivedTypes;
    unsigned long long NumMPIVirtualTopology;
    unsigned long long NumMPIMiscellaneous;
    unsigned long long NumMPIOthers;

    bool count_environment_management(std::string name);
    bool count_point_to_point_communication(std::string name);
    bool count_collective_communication(std::string name);
    bool count_process_group(std::string name);
    bool count_communicators(std::string name);
    bool count_derived_types(std::string name);
    bool count_virtual_topology(std::string name);
    bool count_miscellaneous(std::string name);

    //MPI point to point communication routines
    unsigned long long NumMPIBsend;
    unsigned long long NumMPIBsend_init;
    unsigned long long NumMPIBuffer_attach;
    unsigned long long NumMPIBuffer_detach;
    unsigned long long NumMPICancel;
    unsigned long long NumMPIGet_count;
    unsigned long long NumMPIGet_elements;
    unsigned long long NumMPIIbsend;
    unsigned long long NumMPIIprobe;
    unsigned long long NumMPIIrecv;
    unsigned long long NumMPIIrsend;
    unsigned long long NumMPIIsend;
    unsigned long long NumMPIIssend;
    unsigned long long NumMPIProbe;
    unsigned long long NumMPIRecv;
    unsigned long long NumMPIRecv_init;
    unsigned long long NumMPIRequest_free;
    unsigned long long NumMPIRsend;
    unsigned long long NumMPIRsend_init;
    unsigned long long NumMPISend;
    unsigned long long NumMPISend_init;
    unsigned long long NumMPISendrecv;
    unsigned long long NumMPISendrecv_replace;
    unsigned long long NumMPISsend;
    unsigned long long NumMPISsend_init;
    unsigned long long NumMPIStart;
    unsigned long long NumMPIStartall;
    unsigned long long NumMPITest;
    unsigned long long NumMPITest_cancelled;
    unsigned long long NumMPITestall;
    unsigned long long NumMPITestany;
    unsigned long long NumMPITestsome;
    unsigned long long NumMPIWait;
    unsigned long long NumMPIWaitall;
    unsigned long long NumMPIWaitany;
    unsigned long long NumMPIWaitsome;

    //MPI collective communication routines
    unsigned long long NumMPIAllgather;
    unsigned long long NumMPIAllgatherv;
    unsigned long long NumMPIAllreduce;
    unsigned long long NumMPIAlltoall;
    unsigned long long NumMPIAlltoallv;
    unsigned long long NumMPIBarrier;
    unsigned long long NumMPIBcast;
    unsigned long long NumMPIGather;
    unsigned long long NumMPIGatherv;
    unsigned long long NumMPIOp_create;
    unsigned long long NumMPIOp_free;
    unsigned long long NumMPIReduce;
    unsigned long long NumMPIReduce_scatter;
    unsigned long long NumMPIScan;
    unsigned long long NumMPIScatter;
    unsigned long long NumMPIScatterv;

public:
    MPIstats(Module *M, int thread_id, int processor_id);
    void visit(Instruction &I);
    void JSONdump(JSONmanager *JSONwriter);
};

#endif // LLVM_MPI_STATS__H

