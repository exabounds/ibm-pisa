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

class ILP;
#ifndef LLVM_ILP_H_
#define LLVM_ILP_H_

#include "InstructionAnalysis.h"
#include "InstructionMix.h"
#include "utils.h"
#include "JSONmanager.h"
#include <map>
#include <list>

#include <llvm/Support/raw_os_ostream.h>
#include <iomanip>

#define PRINT_FLOAT_PRECISION 3

struct valueStarIndex {
    unsigned long long lastUsage;
    unsigned long long lastUsageInOrder;
    unsigned long long valueStarCount;
};

struct twoValues {
    unsigned long long value1;
    unsigned long long value2;
};

struct MemoryDep {
    unsigned long long lastStoreCycle;
    unsigned long long lastStoreCycleInOrder;
    unsigned long long lastLoadCycle;
    unsigned long long lastLoadCycleInOrder;
    unsigned long long lastStoreIndex;
    unsigned long long lastLoadIndex;
};

class ILP: public InstructionAnalysis {

public:
    int ilp_type;
    
private:
    int flags;
    int debug_flag;
    unsigned long long valueStarCount;

    // Variables used by ILP scheduler
    unsigned long window_size;
    list<int> scheduler;
    list<int> scheduler_in_order;
    unsigned long long window_cycle;
    unsigned long long window_cycle_in_order;

    unsigned long long MinIssueCycleForCtrl;

    unsigned long long MaxIssueCycle;
    unsigned long long MaxIssueCycleInOrder;

    unsigned long long MaxIssueCycleMemory;
    unsigned long long PreviousIssueCycleMemory;
    unsigned long long MaxIssueCycleInteger;
    unsigned long long PreviousIssueCycleInteger;
    unsigned long long MaxIssueCycleControl;
    unsigned long long PreviousIssueCycleControl;
    unsigned long long MaxIssueCycleFloatingPoint;
    unsigned long long PreviousIssueCycleFloatingPoint;

    // Variables for in-order ILP calculation
    unsigned long long MaxIssueCycleMemoryInOrder;
    unsigned long long PreviousIssueCycleMemoryInOrder;
    unsigned long long MaxIssueCycleIntegerInOrder;
    unsigned long long PreviousIssueCycleIntegerInOrder;
    unsigned long long MaxIssueCycleControlInOrder;
    unsigned long long PreviousIssueCycleControlInOrder;
    unsigned long long MaxIssueCycleFloatingPointInOrder;
    unsigned long long PreviousIssueCycleFloatingPointInOrder;

    double ArithmeticMean;
    unsigned long long accAllInstr;
    unsigned long long accTypeInstr;

    pthread_mutex_t* output_lock;

    // These two vectors replace the matrix with per type distribution
    vector<unsigned long long>  ctrl_distribution;
    vector<unsigned long long>  non_ctrl_distribution;

    BasicBlock *prevBB;
    char compute;

    // Hash map to store the last usage of a given value
    map<Value*, valueStarIndex> LastUsageOperands;
    map<void *, MemoryDep>  LastUsageMemory;

    // Vector that stores the CDF distribution (aggregated over control and non-control)
    map<unsigned long long, unsigned long long> CDF;
    // Vector that stores the CDF distribution (for the ilp-type non-ctrl instructions only)
    map<unsigned long long, unsigned long long> CDF_non_ctrl;
    // Vector that stores the CDF distribution (for the ilp-type ctrl instructions only)
    map<unsigned long long, unsigned long long> CDF_ctrl;

    struct twoValues LookupOperand(Value *op);
    unsigned long long LookupValueStarIndex(Value *op);

    void InsertOperandUsage(Value *op, struct twoValues IssueCycle, bool isNewInstruction, Value* previous=NULL);
    struct twoValues getInstIssueCycle(Instruction &I, struct twoValues IssueCycle);

private:
    bool valueIsConstantLike(const Value* v) const;
    bool valueIsFunctionCall(const Value* v) const;

public:
    ILP(Module *M, int thread_id, int processor_id, int flags, int ilp_type,
        int window_size, int debug_flag, pthread_mutex_t *output_lock);

    void visit(Instruction &I);
    void JSONdump(JSONmanager *JSONwriter, InstructionMix *mix);
    void updateILPforCall(Value *returnInstruction, Value *callInstruction);
    void updateILPIssueCycle(int type, Value *I, void *real_addr, unsigned long long issue_cycle);
};

#endif
