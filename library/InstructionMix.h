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

class InstructionMix;

#ifndef LLVM_INSTRUCTION_MIX__H
#define LLVM_INSTRUCTION_MIX__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "JSONmanager.h"
#include <llvm/Support/raw_os_ostream.h>
#include <sstream>

class InstructionMix: public InstructionAnalysis, public InstVisitor<InstructionMix> {
    friend class InstVisitor<InstructionMix>;

    #define HANDLE_INST(N, OPCODE, CLASS) \
    void visit##OPCODE(CLASS &) { ++Num##OPCODE##Inst;  ++NumTotalInsts;}

    #include "llvm/IR/Instruction.def"

    void visitInstruction(Instruction &I) {
        errs() << "Instruction mix: unknown instruction " << I;
        llvm_unreachable(0);
    }

public:
    InstructionMix(Module *M, int thread_id, int processor_id);

    void visitMemoryInstruction(Instruction &I);
    void visitBinaryArithBitwiseCmpConvertSelectInstruction(Instruction &I);

    #define HANDLE_INST(N, OPCODE, CLASS) \
    unsigned long long Num ## OPCODE ## Inst;

    #include "llvm/IR/Instruction.def"

    void JSONdump(JSONmanager *JSONwriter);
    void JSONdump_vectorIncluded(JSONmanager *JSONwriter);
    void JSONdump_partial(JSONmanager *JSONwriter, unsigned inst_type, 
                          int scalar_type, int vector_size, unsigned long long *sum_partial, unsigned isFloat);
    void JSONdump_perType(JSONmanager *JSONwriter,
                          char* inst_type_name, unsigned inst_type_LLVM, unsigned long long inst, unsigned isFloat);
                          

    // Need to distinguish double word (8 bytes) from single word memory
    // acccesses. These memory accesses are not distinguished by the
    // InstructionCounterVisitor, so we need to manually update them. 

    //Floating point type loads
    unsigned long long NumHalfTyLoadInst; //16-bit floating point type 
    unsigned long long NumFloatTyLoadInst; // 32-bit floating point type 
    unsigned long long NumDoubleTyLoadInst; // 64-bit floating point type 
    unsigned long long NumX86_FP80TyLoadInst; //80-bit floating point type (X87) 
    unsigned long long NumFP128TyLoadInst; //128-bit floating point type (112-bit) 
    unsigned long long NumPPC_FP128TyLoadInst; //128-bit floating point type (two 64-bits, PowerPC) 
    unsigned long long NumX86_MMXTyLoadInst; //MMX vectors (64 bits, X86 specific) 
    unsigned long long NumMiscTyLoadInst; // Arbitrary bit width integers 

    //Integer type loads
    unsigned long long Num4BitsLoadIntInst; //4-bit integer type 
    unsigned long long Num8BitsLoadIntInst; // 8-bit integer type 
    unsigned long long Num16BitsLoadIntInst; // 16-bit integer type 
    unsigned long long Num32BitsLoadIntInst; // 32-bit integer type  
    unsigned long long Num64BitsLoadIntInst; // 64-bit integer type 

    //Floating point type stores
    unsigned long long NumHalfTyStoreInst; //16-bit floating point type 
    unsigned long long NumFloatTyStoreInst; // 32-bit floating point type 
    unsigned long long NumDoubleTyStoreInst; // 64-bit floating point type 
    unsigned long long NumX86_FP80TyStoreInst; //80-bit floating point type (X87) 
    unsigned long long NumFP128TyStoreInst; //128-bit floating point type (112-bit) 
    unsigned long long NumPPC_FP128TyStoreInst; //128-bit floating point type (two 64-bits, PowerPC) 
    unsigned long long NumX86_MMXTyStoreInst; //MMX vectors (64 bits, X86 specific) 
    unsigned long long NumMiscTyStoreInst; 	

    //Integer type stores
    unsigned long long Num4BitsStoreIntInst; //4-bit integer type 
    unsigned long long Num8BitsStoreIntInst; // 8-bit integer type 
    unsigned long long Num16BitsStoreIntInst; // 16-bit integer type 
    unsigned long long Num32BitsStoreIntInst; // 32-bit integer type  
    unsigned long long Num64BitsStoreIntInst; // 64-bit integer type 
    unsigned long long NumTotalInsts;

    unsigned long long getNumCtrlInst();
    unsigned long long getNumBrInst();
    unsigned long long getNumSwitchInst();
    unsigned long long getNumRetInst();
    unsigned long long getNumCallInst();
    unsigned long long getNumIndirectBrInst();
    unsigned long long getNumInvokeInst();
    unsigned long long getNumResumeInst();
    unsigned long long getNumUnreachableInst();
    unsigned long long getNumICmpInst();
    unsigned long long getNumFCmpInst();
    unsigned long long getNumPHIInst();
    unsigned long long getNumAddrArithInst();

    unsigned long long getNumMemInst();
    unsigned long long getNumLoadInst();
    unsigned long long getNumStoreInst();
    unsigned long long getNumIntInst();
    unsigned long long getNumFPInst();
    unsigned long long getNumFAddInst();
    unsigned long long getNumFSubInst();
    unsigned long long getNumFMulInst();
    unsigned long long getNumFDivInst();
    unsigned long long getNumFRemInst();
    unsigned long long getNumIntAddInst();
    unsigned long long getNumIntSubInst();
    unsigned long long getNumIntMulInst();
    unsigned long long getNumIntUDivInst();
    unsigned long long getNumIntSDivInst();
    unsigned long long getNumIntURemInst();
    unsigned long long getNumIntSRemInst();
    unsigned long long getNumConversionInst();
    unsigned long long getNumBitwiseInst();
    unsigned long long getNumMiscInst();

    unsigned long long getNumTotalInsts();
    
    // counters[InstructionType][ScalarType][VectorSize][OperandTypeID]
    // E.g.: counters[Instruction::Load][0, 1, 2][0, 2 ...][Type::HalfTyID]
    map<unsigned, map<int, map<int, map<unsigned, unsigned long long> > > > counters;
};

#endif // LLVM_INSTRUCTION_MIX__H

