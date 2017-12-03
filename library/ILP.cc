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

#include "ILP.h"

ILP::ILP(Module *M, int thread_id, int processor_id, int flags, int ilp_type,
         int window_size, int debug_flag, pthread_mutex_t* output_lock) :
    InstructionAnalysis(M, thread_id, processor_id) {
        this->flags = flags;
        this->ilp_type = ilp_type;
        this->debug_flag = debug_flag;
        this->window_size = window_size;
        this->window_cycle = 0;
        this->window_cycle_in_order = 0;
        this->accAllInstr = 0;
        this->accTypeInstr = 0;

        this->processor_id = processor_id;
        this->output_lock = output_lock;

        this->MinIssueCycleForCtrl  = 0;
        this->MaxIssueCycle = 0;
        this->MaxIssueCycleInOrder  = 0;
        this->ArithmeticMean = 0;

        this->MaxIssueCycleMemory = 0;
        this->PreviousIssueCycleMemory = 0;
        this->MaxIssueCycleInteger = 0;
        this->PreviousIssueCycleInteger = 0;
        this->MaxIssueCycleControl = 0;
        this->PreviousIssueCycleControl = 0;
        this->MaxIssueCycleFloatingPoint = 0;
        this->PreviousIssueCycleFloatingPoint = 0;
        
        this->MaxIssueCycleMemoryInOrder = 0;
        this->PreviousIssueCycleMemoryInOrder = 0;
        this->MaxIssueCycleIntegerInOrder = 0;
        this->PreviousIssueCycleIntegerInOrder = 0;
        this->MaxIssueCycleControlInOrder = 0;
        this->PreviousIssueCycleControlInOrder = 0;
        this->MaxIssueCycleFloatingPointInOrder = 0;
        this->PreviousIssueCycleFloatingPointInOrder = 0;
        
        prevBB = NULL;
        valueStarCount = 0;
}

void ILP::visit(Instruction &I) {
    struct twoValues IssueCycle;
    static int c = 0;
    unsigned InstType = this->getInstType(I);
    raw_os_ostream myCout(cerr);

    IssueCycle.value1 = 0;
    IssueCycle.value2 = 0;

    // MPI_Test
    // If ILP scheduler is active, the minimum issue cycle is window_cycle
    if (window_size) {
        IssueCycle.value1 = window_cycle;
        IssueCycle.value2 = window_cycle_in_order;
    }

    // 'getInstIssueCycle' function computes the IssueCycle for the current instruction. 
    // The IssueCycle can be modified based on analysis arguments
    // restrictions, until the end of this function.
    // This is NOT the final value of the IssueCycle variable.

    IssueCycle = getInstIssueCycle(I, IssueCycle);

    // impose in-order execution
    if (IssueCycle.value2 < MaxIssueCycleInOrder)
        IssueCycle.value2 = MaxIssueCycleInOrder;

    if (isa<PHINode>(I))
        return;

    if ((flags & ANALYZE_ILP_IGNORE_CTRL) && (InstType == CTRL_TYPE))
        return;

    if ((flags & ANALYZE_ILP_CTRL) && (InstType == CTRL_TYPE)) {
        // We need to serialize the IssueCycle of the control instructions.
        IssueCycle.value1 = max(IssueCycle.value1, MinIssueCycleForCtrl);
        MinIssueCycleForCtrl = IssueCycle.value1 + 1;
    }

    // We also insert the mapping between I and the computed IssueCycle.
    // This is useful to extract the IssueCycle of a given return instruction
    // in order to set the same IssueCycle for the register that contains
    // the value returned by a call instruction.
    InsertOperandUsage(&I, IssueCycle, false);

    MaxIssueCycle = max(MaxIssueCycle, IssueCycle.value1);
    MaxIssueCycleInOrder = max(MaxIssueCycleInOrder, IssueCycle.value2);

    if (isOfMemoryType(I)) {
        if (IssueCycle.value1 > PreviousIssueCycleMemory) MaxIssueCycleMemory++;
        PreviousIssueCycleMemory = max(IssueCycle.value1, PreviousIssueCycleMemory);
        if (IssueCycle.value2 > PreviousIssueCycleMemoryInOrder) MaxIssueCycleMemoryInOrder++;
        PreviousIssueCycleMemoryInOrder = max(IssueCycle.value2, PreviousIssueCycleMemoryInOrder);
    } else if (isOfIntegerType(I)) {
        if (IssueCycle.value1 > PreviousIssueCycleInteger) MaxIssueCycleInteger++;
        PreviousIssueCycleInteger = max(IssueCycle.value1, PreviousIssueCycleInteger);
        if (IssueCycle.value2 > PreviousIssueCycleIntegerInOrder) MaxIssueCycleIntegerInOrder++;
        PreviousIssueCycleIntegerInOrder = max(IssueCycle.value2, PreviousIssueCycleIntegerInOrder);
    } else if (isOfControlType(I)) {
        if (IssueCycle.value1 > PreviousIssueCycleControl) MaxIssueCycleControl++;
        PreviousIssueCycleControl = max(IssueCycle.value1, PreviousIssueCycleControl);
        if (IssueCycle.value2 > PreviousIssueCycleControlInOrder) MaxIssueCycleControlInOrder++;
        PreviousIssueCycleControlInOrder = max(IssueCycle.value2, PreviousIssueCycleControlInOrder);
    } else if (isOfFloatingPointType(I)) {
        if (IssueCycle.value1 > PreviousIssueCycleFloatingPoint) MaxIssueCycleFloatingPoint++;
        PreviousIssueCycleFloatingPoint = max(IssueCycle.value1, PreviousIssueCycleFloatingPoint);
        if (IssueCycle.value2 > PreviousIssueCycleFloatingPointInOrder) MaxIssueCycleFloatingPointInOrder++;
        PreviousIssueCycleFloatingPointInOrder = max(IssueCycle.value2, PreviousIssueCycleFloatingPointInOrder);
    }

    // Memorise the basic block of the current instruction.
    // If the next instruction will be a 'phi' instruction, it will need
    // this in order to know which basic block was previously executed.
    prevBB = I.getParent();

    // Introduce ILP scheduler
    if (window_size) {
        
        scheduler.push_back(IssueCycle.value1);
        while (scheduler.size() == window_size) {
            // Full capacity; release instructions
            scheduler.remove(window_cycle);
            window_cycle++;
        }
        
        scheduler_in_order.push_back(IssueCycle.value2);
        while (scheduler_in_order.size() == window_size) {
            // Full capacity; release instructions
            scheduler_in_order.remove(window_cycle_in_order);
            window_cycle_in_order++;
        }
    }

    /* 
    std::size_t found;
    if (!strcmp(I.getOpcodeName(), "call")) {
        // I.print(myCout);
        CallInst *call = cast<CallInst>(&I);
        Function *_call = get_calledFunction(call);
        // assert(_call != NULL);
        if (_call) {
            found = _call->getName().str().find("kmpc");
            if (found != std::string::npos)
                cout << _call->getName().str() << "\n";
        }
    }
    */
    
    if ((flags & PRINT_DEBUG)) {
        pthread_mutex_lock(output_lock);
        cerr << c << " : "; I.print(myCout); cerr << "\n";
        cerr << " procId=" << processor_id 
             << " threadId=" << thread_id 
             << " code=" << I.getOpcodeName()  
             << " cycle(ooo) = " << IssueCycle.value1 
             << " cycle(io) = " << IssueCycle.value2 
             << "\n";
        c++;
        pthread_mutex_unlock(output_lock);
    }

}

void ILP::JSONdump(JSONmanager *JSONwriter, InstructionMix *mix) {
    // computeStatistics();

    JSONwriter->String("ilp");
    JSONwriter->StartArray();

    JSONwriter->StartObject();
    JSONwriter->String("windowSize");
    JSONwriter->Uint64(window_size);
    
    JSONwriter->String("statistics");
    JSONwriter->StartObject();
    
        JSONwriter->String("span");
        JSONwriter->Uint64(MaxIssueCycle + 1);

        JSONwriter->String("arithmetic_mean");
        ArithmeticMean = mix->getNumTotalInsts() / (MaxIssueCycle + 1.0);
        JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_mem");
        JSONwriter->Uint64(MaxIssueCycleMemory);

        JSONwriter->String("arithmetic_mean_mem");

        if (MaxIssueCycleMemory == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = mix->getNumMemInst() / (MaxIssueCycleMemory + 0.0);

        JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_int");
        JSONwriter->Uint64(MaxIssueCycleInteger);

        JSONwriter->String("arithmetic_mean_int");

        if (MaxIssueCycleInteger == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = (mix->getNumIntInst() + mix->getNumBitwiseInst() + 
                                mix->getNumConversionInst() + mix->getNumICmpInst() + 
                                mix->getNumAddrArithInst()) / (MaxIssueCycleInteger + 0.0);
            JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_ctrl");
        JSONwriter->Uint64(MaxIssueCycleControl);

        JSONwriter->String("arithmetic_mean_ctrl");

        if (MaxIssueCycleControl == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = mix->getNumCtrlInst() / (MaxIssueCycleControl + 0.0);

        JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_fp");
        JSONwriter->Uint64(MaxIssueCycleFloatingPoint);

        JSONwriter->String("arithmetic_mean_fp");
        if (MaxIssueCycleFloatingPoint == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = (mix->getNumFPInst() + mix->getNumFCmpInst()) / (MaxIssueCycleFloatingPoint + 0.0);

        JSONwriter->Double(double4(ArithmeticMean));
        
    JSONwriter->EndObject();

        
    // In-order ILP information
    JSONwriter->String("in-order");
    JSONwriter->StartObject();

        JSONwriter->String("span");
        JSONwriter->Uint64(MaxIssueCycleInOrder + 1);

        JSONwriter->String("arithmetic_mean");
        ArithmeticMean = mix->getNumTotalInsts() / (MaxIssueCycleInOrder + 1.0);
        JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_mem");
        JSONwriter->Uint64(MaxIssueCycleMemoryInOrder);

        JSONwriter->String("arithmetic_mean_mem");
        if (MaxIssueCycleMemoryInOrder == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = mix->getNumMemInst() / (MaxIssueCycleMemoryInOrder + 0.0);
        JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_int");
        JSONwriter->Uint64(MaxIssueCycleIntegerInOrder);

        JSONwriter->String("arithmetic_mean_int");
        if (MaxIssueCycleIntegerInOrder == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = (mix->getNumIntInst() + mix->getNumBitwiseInst() + 
                                mix->getNumConversionInst() + mix->getNumICmpInst() + 
                                mix->getNumAddrArithInst()) / (MaxIssueCycleIntegerInOrder + 0.0);
        JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_ctrl");
        JSONwriter->Uint64(MaxIssueCycleControlInOrder);

        JSONwriter->String("arithmetic_mean_ctrl");
        if (MaxIssueCycleControlInOrder == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = mix->getNumCtrlInst() / (MaxIssueCycleControlInOrder + 0.0);
        JSONwriter->Double(double4(ArithmeticMean));

        JSONwriter->String("span_fp");
        JSONwriter->Uint64(MaxIssueCycleFloatingPointInOrder);

        JSONwriter->String("arithmetic_mean_fp");
        if (MaxIssueCycleFloatingPointInOrder == 0)
            ArithmeticMean = 0;
        else
            ArithmeticMean = (mix->getNumFPInst() + mix->getNumFCmpInst()) / (MaxIssueCycleFloatingPointInOrder + 0.0);
        JSONwriter->Double(double4(ArithmeticMean));

    JSONwriter->EndObject();
            
    JSONwriter->EndObject();
    JSONwriter->EndArray();
}

struct twoValues ILP::LookupOperand(Value *op) {
    struct twoValues IssueCycle;
    IssueCycle.value1 = 0;
    IssueCycle.value2 = 0;

    map<Value*, valueStarIndex>::iterator it;

    it = LastUsageOperands.find(op);
    if(it != LastUsageOperands.end()) {
        IssueCycle.value1 = it->second.lastUsage;
        IssueCycle.value2 = it->second.lastUsageInOrder;
    }

    return IssueCycle;
}

// Used to print a trace with all instructions and their register dependencies
unsigned long long ILP::LookupValueStarIndex(Value *op) {
    map<Value*, valueStarIndex>::iterator it;
    unsigned long long valueStar = 0;

    it = LastUsageOperands.find(op);
    if(it != LastUsageOperands.end())
        valueStar = it->second.valueStarCount;

    return valueStar;
}

void ILP::InsertOperandUsage(Value *op, struct twoValues IssueCycle, bool isNewInstruction, Value* previousOpForCount) {
    //cout << "Inserting usage for op=" << op << " at cycle=" << IssueCycle << endl;
    map <Value*, valueStarIndex>::iterator it;
    valueStarIndex newInstruction;
    unsigned long long oldValue = 0;

    // Constant values are not operands/registers. Skip.
    if (dyn_cast<Constant>(op)) {
        return;
    }

    it = LastUsageOperands.find(op);
    if (it != LastUsageOperands.end())  {
        oldValue = it->second.valueStarCount;
        // fprintf(stderr,"thread %d removing %p at cycle %llu in structure %p\n", omp_get_thread_num(), it->first, IssueCycle, &LastUsageOperands);
        // LastUsageOperands.erase(it);
    }

    if (isNewInstruction) {
        newInstruction.valueStarCount = valueStarCount;
        valueStarCount++;
    } else if (previousOpForCount != NULL) {
        newInstruction.valueStarCount = LookupValueStarIndex(previousOpForCount);
    } else {
        newInstruction.valueStarCount = oldValue;
    }

    newInstruction.lastUsage = IssueCycle.value1;
    newInstruction.lastUsageInOrder = IssueCycle.value2;

    // fprintf(stderr,"thread %d adding %p at cyle %llu in structure %p\n", omp_get_thread_num(), op, IssueCycle, &LastUsageOperands);
    // LastUsageOperands.insert(pair<Value*,valueStarIndex>(op, newInstruction)); 
    LastUsageOperands[op] = newInstruction;
}

bool ILP::valueIsConstantLike(const Value* v) const {
    return (dyn_cast<Constant>(v) || dyn_cast<BasicBlock>(v) || isa<PHINode>(*v));
}

bool ILP::valueIsFunctionCall(const Value* v) const {
    if (!dyn_cast<Instruction>(v)) return false;
    return dyn_cast<Instruction>(v)->getOpcode() == Instruction::Call;
}

// To be enabled to generate a trace with all instructions and their register dependencies
// #define TRACE_DEPENDENCIES 1

//#include <typeinfo>
struct twoValues ILP::getInstIssueCycle(Instruction &I, struct twoValues IssueCycle) {
    unsigned long long OpCode = I.getOpcode();
    raw_os_ostream myCout(cerr);

    map <Value*, unsigned long long>::iterator it;
    map <void *, struct MemoryDep>::iterator itMemory;

    if (isa<PHINode>(I)) {
        // Make connection between old Value * and new Value *.
        // This is the purpose of PHI instructions.
        PHINode *PN = cast<PHINode>(&I);

        // If an instruction is using the result register of a phi function,
        // then the cycle usage of that register must be accurately computed.
        // E.g., if the argument corresponding to a given path in the phi
        // instruction is a constant, then the cycle of the phi result must 
        // be zero, i.e., the cycle of the constant. The same if instead of
        // a constant, there is a basic block label.
        for (unsigned long long i = 0; i < PN->getNumIncomingValues(); i++) {
            if (PN->getIncomingBlock(i) == prevBB) {
                Value *v = PN->getIncomingValue(i);
                struct twoValues ArgumentUsage = LookupOperand(v);
                if (!valueIsConstantLike(v) && !valueIsFunctionCall(v)) {
                    ++ArgumentUsage.value1;
                    ++ArgumentUsage.value2;
                    ArgumentUsage.value2 = max(ArgumentUsage.value2, MaxIssueCycleInOrder);
                }
                InsertOperandUsage(&I, ArgumentUsage, false, v);
                IssueCycle.value1 = max(IssueCycle.value1, ArgumentUsage.value1);
                IssueCycle.value2 = max(IssueCycle.value2, ArgumentUsage.value2);
                break;
            }
        }
    } else if (OpCode == Instruction::Call) {
        struct twoValues ArgumentUsage;
        CallInst *CI = cast<CallInst>(&I);
        Function *F = get_calledFunction(CI);

#ifdef TRACE_DEPENDENCIES
        I.print(myCout);
        cerr << " : ";
#endif

        // If the function is in reality a pointer to a function,
        // we need to extract the address and identify which
        // function is actually called.
        if (!F) {
            Value *v = (Function *)CI->getCalledValue();
            void *addr = getMemoryAddress((Instruction *)v, thread_id);
            if (addr) addr = (void *)*(unsigned long long *)addr;
            F = getFunctionAddress(addr);
            if (!F) {
                CallSite CS = CallSite(&I);
                F = CS.getCalledFunction();
                if (!F) {
                    
#ifdef TRACE_DEPENDENCIES
                    cerr << " pointer to function not found " << " - " << LookupValueStarIndex(v) << "\n";
#endif
                    return IssueCycle;
                }
            }
        }

        if (CI->getNumArgOperands() > 0) {
            Function::arg_iterator f_it = F->arg_begin();
            Function::arg_iterator f_end = F->arg_end();

            for (unsigned long long i = 0; i < CI->getNumArgOperands(); i++) {
                ArgumentUsage = LookupOperand(CI->getArgOperand(i));

                if (!valueIsConstantLike(CI->getArgOperand(i))) {
                    IssueCycle.value1 = max(IssueCycle.value1, 1+ArgumentUsage.value1);
                    IssueCycle.value2 = max(IssueCycle.value2, 1+ArgumentUsage.value2);
                    IssueCycle.value2 = max(IssueCycle.value2, MaxIssueCycleInOrder);
                    
#ifdef TRACE_DEPENDENCIES
                    cerr << LookupValueStarIndex(CI->getArgOperand(i)) << " ";
#endif

                } else if (isa<PHINode>(*(CI->getArgOperand(i)))) {
                    IssueCycle.value1 = max(IssueCycle.value1, ArgumentUsage.value1);
                    IssueCycle.value2 = max(IssueCycle.value2, ArgumentUsage.value2);
                    IssueCycle.value2 = max(IssueCycle.value2, MaxIssueCycleInOrder);
                    
#ifdef TRACE_DEPENDENCIES
                    cerr << LookupValueStarIndex(CI->getArgOperand(i)) << " ";
#endif
                }

                if (f_it != f_end) {
                    // Update usage of function parameters
                    InsertOperandUsage((Argument*)f_it, ArgumentUsage, false, CI->getArgOperand(i));
                    ++f_it;
                }
            }
        } 

        InsertOperandUsage(&I, IssueCycle, true);

#ifdef TRACE_DEPENDENCIES
        cerr << " : " << LookupValueStarIndex(&I) << "\n";
#endif

    } else if (OpCode != Instruction::Store && OpCode != Instruction::Load) { 
        int NumOperands = I.getNumOperands();

#ifdef TRACE_DEPENDENCIES
        I.print(myCout);
        cerr << " : ";
#endif

        if (NumOperands > 0) {
            for (int i = 0; i < NumOperands; i++){
                Value *v = I.getOperand(i);
                
                //cout << "Operand type=" << typeid(*v).name() << endl;
                if (!valueIsConstantLike(v)) {
                    struct twoValues tmp = LookupOperand(v);
                    IssueCycle.value1 = max(IssueCycle.value1, 1+tmp.value1);
                    IssueCycle.value2 = max(IssueCycle.value2, 1+tmp.value2);
                    IssueCycle.value2 = max(IssueCycle.value2, MaxIssueCycleInOrder);
                    
#ifdef TRACE_DEPENDENCIES
                    cerr << LookupValueStarIndex(v) << " ";
#endif

                } else if (isa<PHINode>(*v)) {
                    struct twoValues tmp = LookupOperand(v);
                    IssueCycle.value1 = max(IssueCycle.value1, tmp.value1);
                    IssueCycle.value2 = max(IssueCycle.value2, tmp.value2);
                    IssueCycle.value2 = max(IssueCycle.value2, MaxIssueCycleInOrder);
                    
#ifdef TRACE_DEPENDENCIES
                    cerr << LookupValueStarIndex(v) << " ";
#endif
                }
            }
        }

        InsertOperandUsage(&I, IssueCycle, true);

#ifdef TRACE_DEPENDENCIES
        cerr << " : " << LookupValueStarIndex(&I) << "\n";
#endif

    } else if (OpCode == Instruction::Load) {

#ifdef TRACE_DEPENDENCIES
        I.print(myCout);
        cerr << " : ";
#endif

        struct twoValues op1 = LookupOperand(I.getOperand(0));
        IssueCycle.value1 = max(IssueCycle.value1, op1.value1+1);
        IssueCycle.value2 = max(IssueCycle.value2, op1.value2+1);
        IssueCycle.value2 = max(IssueCycle.value2, MaxIssueCycleInOrder);

#ifdef TRACE_DEPENDENCIES
        cerr  << LookupValueStarIndex(I.getOperand(0)) << " ";
#endif

        void *Address = getMemoryAddress(&I, thread_id);
        itMemory = LastUsageMemory.find(Address);
        if (itMemory != LastUsageMemory.end()) {
            // If the operand was previously used
            unsigned long long maxStoreCycle = itMemory->second.lastStoreCycle;
            unsigned long long maxLoadCycle = itMemory->second.lastLoadCycle;
            IssueCycle.value1 = max(IssueCycle.value1, maxStoreCycle+1);
            
            unsigned long long maxStoreCycleInOrder = itMemory->second.lastStoreCycleInOrder;
            unsigned long long maxLoadCycleInOrder = itMemory->second.lastLoadCycleInOrder;
            IssueCycle.value2 = max(IssueCycle.value2, maxStoreCycleInOrder+1);
            IssueCycle.value2 = max(IssueCycle.value2, MaxIssueCycleInOrder);
            
            InsertOperandUsage(&I, IssueCycle, true);
            // LastUsageMemory.erase(itMemory);
            maxLoadCycle = max(maxLoadCycle, IssueCycle.value1);
            maxLoadCycleInOrder = max(maxLoadCycleInOrder, IssueCycle.value2);

            // do not update
            struct MemoryDep tmp;
            tmp.lastStoreCycle = maxStoreCycle;
            tmp.lastLoadCycle = maxLoadCycle;
            tmp.lastStoreCycleInOrder = maxStoreCycleInOrder;
            tmp.lastLoadCycleInOrder = maxLoadCycleInOrder;
            tmp.lastStoreIndex = itMemory->second.lastStoreIndex;
            tmp.lastLoadIndex = LookupValueStarIndex(&I);

            // LastUsageMemory.erase(itMemory);

#ifdef TRACE_DEPENDENCIES
            cerr << itMemory->second.lastStoreIndex << " ";
#endif

            pair<void *, struct MemoryDep > map(Address, tmp);
            // LastUsageMemory.insert(map);
            itMemory->second = tmp;
        } else {
            // should not do a load before anything was stored at that address
            // however it still happens in the LLVM IR code. for instance, 
            // when memory is initialized by a library call (e.g. memset)
            // or for global variables: global int32* optarg; %a = load i32** @optarg
            InsertOperandUsage(&I, IssueCycle, true);

            // TODO: check how many times this happens
            struct MemoryDep tmp;
            tmp.lastStoreCycle = 0;
            tmp.lastLoadCycle = IssueCycle.value1;
            tmp.lastStoreCycleInOrder = 0;
            tmp.lastLoadCycleInOrder = IssueCycle.value2;
            tmp.lastStoreIndex = 0;
            tmp.lastLoadIndex = LookupValueStarIndex(&I);

            pair<void *,struct MemoryDep> map(Address, tmp);
            LastUsageMemory.insert(map);
        }

#ifdef TRACE_DEPENDENCIES
        cerr << " : " << LookupValueStarIndex(&I) << "\n";
#endif

    } else if (OpCode == Instruction::Store) {

#ifdef TRACE_DEPENDENCIES
        I.print(myCout);
        cerr << " : ";
#endif

        struct twoValues op1 = LookupOperand(I.getOperand(0));
        struct twoValues op2 = LookupOperand(I.getOperand(1));
        IssueCycle.value1 = max(IssueCycle.value1, op1.value1+1);
        IssueCycle.value1 = max(IssueCycle.value1, op2.value1+1);
        IssueCycle.value2 = max(IssueCycle.value2, op1.value2+1);
        IssueCycle.value2 = max(IssueCycle.value2, op2.value2+1);
        IssueCycle.value2 = max(IssueCycle.value2, MaxIssueCycleInOrder);

#ifdef TRACE_DEPENDENCIES
        if (processor_id == 1){
            // cerr << "op1=" << op1 << " op2=" << op2 << "\n";
            // cerr << LookupValueStarIndex(I.getOperand(0)) << " " << LookupValueStarIndex(I.getOperand(1)) << " ";
        }
#endif

        void *Address = getMemoryAddress(&I, thread_id);
        itMemory = LastUsageMemory.find(Address);

        if (itMemory != LastUsageMemory.end()) { 
            unsigned long long maxStoreCycle = itMemory->second.lastStoreCycle;
            unsigned long long maxLoadCycle = itMemory->second.lastLoadCycle;
            unsigned long long maxStoreCycleInOrder = itMemory->second.lastStoreCycleInOrder;
            unsigned long long maxLoadCycleInOrder = itMemory->second.lastLoadCycleInOrder;
            
#ifdef TRACE_DEPENDENCIES
            if (processor_id == 1)
                cerr << "lastStore=" << maxStoreCycle << " lastLoad=" << maxLoadCycle << "\n";
#endif

            IssueCycle.value1 = max(IssueCycle.value1, maxStoreCycle+1);
            IssueCycle.value1 = max(IssueCycle.value1, maxLoadCycle+1);
            maxStoreCycle = max(maxStoreCycle, IssueCycle.value1);

            IssueCycle.value2 = max(IssueCycle.value2, maxStoreCycleInOrder+1);
            IssueCycle.value2 = max(IssueCycle.value2, maxLoadCycleInOrder+1);
            maxStoreCycleInOrder = max(maxStoreCycleInOrder, IssueCycle.value2);
            
            InsertOperandUsage(&I, IssueCycle, true);
            InsertOperandUsage(I.getOperand(1), IssueCycle, false, &I);

            struct MemoryDep tmp;
            tmp.lastStoreCycle = maxStoreCycle;
            tmp.lastLoadCycle = maxLoadCycle;
            tmp.lastStoreCycleInOrder = maxStoreCycleInOrder;
            tmp.lastLoadCycleInOrder = maxLoadCycleInOrder;
            tmp.lastLoadIndex = itMemory->second.lastLoadIndex;
            tmp.lastStoreIndex = LookupValueStarIndex(&I);

            // LastUsageMemory.erase(itMemory);
            // maxStoreCycle = max(maxStoreCycle, IssueCycle);
            // pair<void *, struct MemoryDep> map(Address, tmp);
            // LastUsageMemory.insert(map);
            itMemory->second = tmp;

        } else {
            InsertOperandUsage(&I, IssueCycle, true);
            InsertOperandUsage(I.getOperand(1), IssueCycle, false, &I);

            struct MemoryDep tmp;
            tmp.lastStoreCycle = IssueCycle.value1;
            tmp.lastLoadCycle = 0;
            tmp.lastStoreCycleInOrder = IssueCycle.value2;
            tmp.lastLoadCycleInOrder = 0;
            tmp.lastLoadIndex = 0;
            tmp.lastStoreIndex = LookupValueStarIndex(&I);

            pair<void *, struct MemoryDep> map(Address, tmp);
            LastUsageMemory.insert(map);
        }        

        // InsertOperandUsage(&I, IssueCycle, true);
        // Also update the Value * of the destination
        // InsertOperandUsage(I.getOperand(1), IssueCycle, false, &I);

    }
    
    return IssueCycle;
}

void ILP::updateILPforCall(Value *returnInstruction, Value *callInstruction) {
    struct twoValues returnIssueCycle = LookupOperand(returnInstruction);
    struct twoValues callIssueCycle = LookupOperand(callInstruction);

    if (returnIssueCycle.value1 >= callIssueCycle.value1) {
        if (returnIssueCycle.value2 >= callIssueCycle.value2) {
            InsertOperandUsage(callInstruction, returnIssueCycle, false, returnInstruction);
        } else {
            struct twoValues newIssueCycle;
            newIssueCycle.value1 = returnIssueCycle.value1;
            newIssueCycle.value2 = callIssueCycle.value2;
            InsertOperandUsage(callInstruction, newIssueCycle, false, returnInstruction);
        }
    }
    else 
        if (returnIssueCycle.value2 >= callIssueCycle.value2) {
            // TODO: double-check the accuracy
            struct twoValues newIssueCycle;
            newIssueCycle.value1 = callIssueCycle.value1;
            newIssueCycle.value2 = returnIssueCycle.value2;
            InsertOperandUsage(callInstruction, newIssueCycle, false, returnInstruction);
        }
}

void ILP::updateILPIssueCycle(int type, Value *I, void *real_addr, unsigned long long issue_cycle) {
/*
    map <void *, struct MemoryDep>::iterator itMemory;

    // FIXME!! Th valueStarIndex of the buffer I should be updated to thevalueStarIndex of the new intruction triggering the issue cycle update!
    InsertOperandUsage(I, issue_cycle, false);

    itMemory = LastUsageMemory.find(real_addr);
    if (itMemory != LastUsageMemory.end()) {
        unsigned long long maxStoreCycle = itMemory->second.lastStoreCycle;
        unsigned long long maxLoadCycle = itMemory->second.lastLoadCycle;

        if (type == READ_OPERATION) {
            // The current process is doing a MPI_Send call.
            // The data is performing a read operation.
            // We must update the memory access, acordingly.
            // LastUsageMemory.erase(itMemory);
            issue_cycle = max(maxLoadCycle, issue_cycle);
            // fprintf(stderr, "%p maxStore=%llu maxLoad=%llu new_load=%llu\n", real_addr, maxStoreCycle, maxLoadCycle, issue_cycle);

                        struct MemoryDep tmp;
                        tmp.lastStoreCycle = maxStoreCycle;
                        tmp.lastLoadCycle = issue_cycle;
            // FIXME
                        tmp.lastLoadIndex = 0;
                        tmp.lastStoreIndex = 0;

            // pair<unsigned long long, unsigned long long> tmp(maxStoreCycle, issue_cycle);
            pair<void *, struct MemoryDep > map(real_addr, tmp);
            // LastUsageMemory.insert(map);
            itMemory->second = tmp;
        } else if (type == WRITE_OPERATION) {
            // The current process is doing a MPI_Recv call.
            // The data is performing a write operation.
            // We must update the memory access, acordingly.
            // LastUsageMemory.erase(itMemory);
            issue_cycle = max(maxStoreCycle, issue_cycle);
            // fprintf(stderr, "%p maxStore=%llu maxLoad=%llu new_store=%llu\n", real_addr, maxStoreCycle, maxLoadCycle, issue_cycle);

            struct MemoryDep tmp;
            tmp.lastStoreCycle = issue_cycle;
            tmp.lastLoadCycle = maxLoadCycle;
            // FIXME
            tmp.lastLoadIndex = 0;
            tmp.lastStoreIndex = 0;

            // pair<unsigned long long, unsigned long long> tmp(issue_cycle, maxLoadCycle);
            pair<void *, struct MemoryDep> map(real_addr, tmp);
            // LastUsageMemory.insert(map);
            itMemory->second = tmp;
        }
    } else {
        if (type == READ_OPERATION) {
            // The current process is doing a MPI_Send call.
            // The data is performing a read operation.
            struct MemoryDep tmp;
            tmp.lastStoreCycle = 0;
            tmp.lastLoadCycle = issue_cycle;
            // FIXME
            tmp.lastLoadIndex = 0;
            tmp.lastStoreIndex = 0;

            // pair<unsigned long long, unsigned long long> tmp(0, issue_cycle);
            pair<void *, struct MemoryDep> map(real_addr, tmp);
            LastUsageMemory.insert(map);
        } else if (type == WRITE_OPERATION) {
            // The current process is doing a MPI_Recv call.
            // The data is performing a write operation.

            struct MemoryDep tmp;
            tmp.lastStoreCycle = issue_cycle;
            tmp.lastLoadCycle = 0;
            // FIXME
            tmp.lastLoadIndex = 0;
            tmp.lastStoreIndex = 0;

            // pair<unsigned long long, unsigned long long> tmp(issue_cycle, 0);
            pair<void *, struct MemoryDep> map(real_addr, tmp);
            LastUsageMemory.insert(map);
        }
    }*/
}
