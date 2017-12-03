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

#include "InstructionMix.h"

InstructionMix::InstructionMix(Module *M, int thread_id, int processor_id) :
    InstructionAnalysis(M, thread_id, processor_id) {

#define HANDLE_INST(N, OPCODE, CLASS) \
    Num ## OPCODE ## Inst = 0;

#include "llvm/IR/Instruction.def"
    NumHalfTyLoadInst       = 0;
    NumFloatTyLoadInst      = 0;
    NumDoubleTyLoadInst     = 0;
    NumX86_FP80TyLoadInst   = 0;
    NumFP128TyLoadInst      = 0;
    NumPPC_FP128TyLoadInst  = 0;
    NumX86_MMXTyLoadInst    = 0;
    Num4BitsLoadIntInst     = 0;
    Num8BitsLoadIntInst     = 0;
    Num16BitsLoadIntInst    = 0;
    Num32BitsLoadIntInst    = 0;
    Num64BitsLoadIntInst    = 0;
    NumMiscTyLoadInst       = 0;
    NumHalfTyStoreInst      = 0;
    NumFloatTyStoreInst     = 0;
    NumDoubleTyStoreInst    = 0;
    NumX86_FP80TyStoreInst  = 0;
    NumFP128TyStoreInst     = 0;
    NumPPC_FP128TyStoreInst = 0;
    NumX86_MMXTyStoreInst   = 0;
    Num4BitsStoreIntInst    = 0;
    Num8BitsStoreIntInst    = 0;
    Num16BitsStoreIntInst   = 0;
    Num32BitsStoreIntInst   = 0;
    Num64BitsStoreIntInst   = 0;
    NumMiscTyStoreInst      = 0;

    NumTotalInsts           = 0;
}

// CONTROL instructions
unsigned long long InstructionMix::getNumCtrlInst() {
    return (NumRetInst + NumBrInst + NumSwitchInst + NumIndirectBrInst +
            NumInvokeInst + NumResumeInst + NumUnreachableInst + NumCallInst);
}

unsigned long long InstructionMix::getNumRetInst() {
    return NumRetInst;
}

unsigned long long InstructionMix::getNumBrInst() {
    return NumBrInst;
}

unsigned long long InstructionMix::getNumSwitchInst() {
    return NumSwitchInst;
}

unsigned long long InstructionMix::getNumCallInst() {
    return NumCallInst;
}

unsigned long long InstructionMix::getNumIndirectBrInst() {
    return NumIndirectBrInst;
}

unsigned long long InstructionMix::getNumInvokeInst() {
    return NumInvokeInst;
}

unsigned long long InstructionMix::getNumResumeInst() {
    return NumResumeInst;
}

unsigned long long InstructionMix::getNumUnreachableInst() {
    return NumUnreachableInst;
}


// INTEGER instructions
unsigned long long InstructionMix::getNumIntInst() {
    return (NumAddInst + NumSubInst + NumMulInst + NumUDivInst +
            NumSDivInst + NumURemInst + NumSRemInst);
}

unsigned long long InstructionMix::getNumIntAddInst() {
    return NumAddInst;
}

unsigned long long InstructionMix::getNumIntSubInst() {
    return NumSubInst;
}

unsigned long long InstructionMix::getNumIntMulInst() {
    return NumMulInst;
}

unsigned long long InstructionMix::getNumIntUDivInst() {
    return NumUDivInst;
}

unsigned long long InstructionMix::getNumIntSDivInst() {
    return NumSDivInst;
}

unsigned long long InstructionMix::getNumIntURemInst() {
    return NumURemInst;
}

unsigned long long InstructionMix::getNumIntSRemInst() {
    return NumSRemInst;
}

unsigned long long InstructionMix::getNumICmpInst() {
    return NumICmpInst;
}

unsigned long long InstructionMix::getNumAddrArithInst() {
    return NumAllocaInst + NumGetElementPtrInst;
}


// FLOATING-POINT instructions
unsigned long long InstructionMix::getNumFCmpInst() {
    return NumFCmpInst;
}


// PHI instructions
unsigned long long InstructionMix::getNumPHIInst() {
    return NumPHIInst;
}


// MEMORY instructions
unsigned long long
InstructionMix::getNumMemInst() {
    return (NumLoadInst + NumStoreInst + NumAtomicCmpXchgInst + NumAtomicRMWInst);
//  return (NumAllocaInst + NumLoadInst + NumStoreInst + NumAtomicCmpXchgInst +
//          NumAtomicRMWInst + NumFenceInst +
//          NumGetElementPtrInst);
}


unsigned long long InstructionMix::getNumLoadInst() {
    return NumLoadInst;
}


unsigned long long InstructionMix::getNumStoreInst() {
    return NumStoreInst;
}

// FLOATING-POINT instructions
unsigned long long InstructionMix::getNumFPInst() {
    return (NumFAddInst + NumFSubInst + NumFMulInst +
            NumFDivInst + NumFRemInst);
}


unsigned long long InstructionMix::getNumFAddInst() {
    return NumFAddInst;
}


unsigned long long InstructionMix::getNumFSubInst() {
    return NumFSubInst;
}


unsigned long long InstructionMix::getNumFMulInst() {
    return NumFMulInst;
}


unsigned long long InstructionMix::getNumFDivInst() {
    return NumFDivInst;
}


unsigned long long InstructionMix::getNumFRemInst() {
    return NumFRemInst;
}

// INTEGER instructions
unsigned long long InstructionMix::getNumConversionInst() {
    return(NumTruncInst + NumZExtInst + NumSExtInst +
            NumFPTruncInst + NumFPExtInst+ NumFPToUIInst +
            NumFPToSIInst + NumUIToFPInst + NumSIToFPInst +
            NumIntToPtrInst + NumPtrToIntInst + NumBitCastInst + NumAddrSpaceCastInst);
}


unsigned long long InstructionMix::getNumBitwiseInst() {
    return (NumAndInst + NumOrInst + NumXorInst + NumShlInst +
            NumLShrInst + NumAShrInst);
}

// MISC instructions - NOT used anymore
unsigned long long InstructionMix::getNumMiscInst() {
    return (NumVAArgInst + NumExtractElementInst +
            NumInsertElementInst + NumShuffleVectorInst +
            NumExtractValueInst + NumInsertValueInst);
}

unsigned long long InstructionMix::getNumTotalInsts() {
    return NumTotalInsts;
}

void InstructionMix::JSONdump(JSONmanager *JSONwriter) {
    JSONwriter->String("instructionMix");
    JSONwriter->StartObject();

    unsigned long long CtrlInst     = getNumCtrlInst();
    unsigned long long IntInst      = getNumIntInst();
    unsigned long long FPInst       = getNumFPInst();
    unsigned long long FAddInst     = getNumFAddInst();
    unsigned long long FSubInst     = getNumFSubInst();
    unsigned long long FMulInst     = getNumFMulInst();
    unsigned long long FDivInst     = getNumFDivInst();
    unsigned long long FRemInst     = getNumFRemInst();
    unsigned long long IntAddInst   = getNumIntAddInst();
    unsigned long long IntSubInst   = getNumIntSubInst();
    unsigned long long IntMulInst   = getNumIntMulInst();
    unsigned long long IntUDivInst  = getNumIntUDivInst();
    unsigned long long IntSDivInst  = getNumIntSDivInst();
    unsigned long long IntURemInst  = getNumIntURemInst();
    unsigned long long IntSRemInst  = getNumIntSRemInst();
    unsigned long long MemInst      = getNumMemInst();
    unsigned long long LoadInst     = getNumLoadInst();
    unsigned long long StoreInst    = getNumStoreInst();
    unsigned long long BitwiseInst  = getNumBitwiseInst();
    unsigned long long ConversionInst   = getNumConversionInst();
    unsigned long long MiscInst         = getNumMiscInst();

    JSONwriter->String("instructions_analyzed");
    JSONwriter->Uint64(NumTotalInsts);

    JSONwriter->String("load_instructions");
    JSONwriter->Uint64(LoadInst);

    JSONwriter->String("load_float_16bits_instructions");
    JSONwriter->Uint64(NumHalfTyLoadInst);
    JSONwriter->String("load_float_32bits_instructions");
    JSONwriter->Uint64(NumFloatTyLoadInst);
    JSONwriter->String("load_float_64bits_instructions");
    JSONwriter->Uint64(NumDoubleTyLoadInst);
    JSONwriter->String("load_float_80bits_instructions");
    JSONwriter->Uint64(NumX86_FP80TyLoadInst);
    JSONwriter->String("load_float_128bits_instructions");
    JSONwriter->Uint64(NumFP128TyLoadInst);
    JSONwriter->String("load_float_128bits_powerpc_instructions");
    JSONwriter->Uint64(NumPPC_FP128TyLoadInst);
    JSONwriter->String("load_float_64bits_mmx_instructions");
    JSONwriter->Uint64(NumX86_MMXTyLoadInst);
    JSONwriter->String("load_int_4bits_instructions");
    JSONwriter->Uint64(Num4BitsLoadIntInst);
    JSONwriter->String("load_int_8bits_instructions");
    JSONwriter->Uint64(Num8BitsLoadIntInst);
    JSONwriter->String("load_int_16bits_instructions");
    JSONwriter->Uint64(Num16BitsLoadIntInst);
    JSONwriter->String("load_int_32bits_instructions");
    JSONwriter->Uint64(Num32BitsLoadIntInst);
    JSONwriter->String("load_int_64bits_instructions");
    JSONwriter->Uint64(Num64BitsLoadIntInst);
    JSONwriter->String("load_misc_instructions");
    JSONwriter->Uint64(NumMiscTyLoadInst);

    JSONwriter->String("store_instructions");
    JSONwriter->Uint64(StoreInst);
    JSONwriter->String("store_float_16bits_instructions");
    JSONwriter->Uint64(NumHalfTyStoreInst);
    JSONwriter->String("store_float_32bits_instructions");
    JSONwriter->Uint64(NumFloatTyStoreInst);
    JSONwriter->String("store_float_64bits_instructions");
    JSONwriter->Uint64(NumDoubleTyStoreInst);
    JSONwriter->String("store_float_80bits_instructions");
    JSONwriter->Uint64(NumX86_FP80TyStoreInst);
    JSONwriter->String("store_float_128bits_instructions");
    JSONwriter->Uint64(NumFP128TyStoreInst);
    JSONwriter->String("store_float_128bits_powerpc_instructions");
    JSONwriter->Uint64(NumPPC_FP128TyStoreInst);
    JSONwriter->String("store_float_64bits_mmx_instructions");
    JSONwriter->Uint64(NumX86_MMXTyStoreInst);
    JSONwriter->String("store_int_4bits_instructions");
    JSONwriter->Uint64(Num4BitsStoreIntInst);
    JSONwriter->String("store_int_8bits_instructions");
    JSONwriter->Uint64(Num8BitsStoreIntInst);
    JSONwriter->String("store_int_16bits_instructions");
    JSONwriter->Uint64(Num16BitsStoreIntInst);
    JSONwriter->String("store_int_32bits_instructions");
    JSONwriter->Uint64(Num32BitsStoreIntInst);
    JSONwriter->String("store_int_64bits_instructions");
    JSONwriter->Uint64(Num64BitsStoreIntInst);
    JSONwriter->String("store_misc_instructions");
    JSONwriter->Uint64(NumMiscTyStoreInst);

    JSONwriter->String("atomic_memory_instructions");
    JSONwriter->Uint64(NumAtomicCmpXchgInst + NumAtomicRMWInst);
    JSONwriter->String("atomic_memory_cmpxchg_instructions");
    JSONwriter->Uint64(NumAtomicCmpXchgInst);
    JSONwriter->String("atomic_memory_rmw_instructions");
    JSONwriter->Uint64(NumAtomicRMWInst);

    JSONwriter->String("int_arith_instructions");
    JSONwriter->Uint64(IntInst);
    JSONwriter->String("int_arith_add_instructions");
    JSONwriter->Uint64(IntAddInst);
    JSONwriter->String("int_arith_sub_instructions");
    JSONwriter->Uint64(IntSubInst);
    JSONwriter->String("int_arith_mul_instructions");
    JSONwriter->Uint64(IntMulInst);
    JSONwriter->String("int_arith_udiv_instructions");
    JSONwriter->Uint64(IntUDivInst);
    JSONwriter->String("int_arith_sdiv_instructions");
    JSONwriter->Uint64(IntSDivInst);
    JSONwriter->String("int_arith_urem_instructions");
    JSONwriter->Uint64(IntURemInst);
    JSONwriter->String("int_arith_srem_instructions");
    JSONwriter->Uint64(IntSRemInst);

    JSONwriter->String("int_cmp_instructions");
    JSONwriter->Uint64(NumICmpInst);

    JSONwriter->String("bitwise_instructions");
    JSONwriter->Uint64(BitwiseInst);
    JSONwriter->String("bitwise_and_instructions");
    JSONwriter->Uint64(NumAndInst);
    JSONwriter->String("bitwise_or_instructions");
    JSONwriter->Uint64(NumOrInst);
    JSONwriter->String("bitwise_xor_instructions");
    JSONwriter->Uint64(NumXorInst);
    JSONwriter->String("bitwise_shift_left_instructions");
    JSONwriter->Uint64(NumShlInst);
    JSONwriter->String("bitwise_logical_shift_right_instructions");
    JSONwriter->Uint64(NumLShrInst);
    JSONwriter->String("bitwise_arith_shift_right_instructions");
    JSONwriter->Uint64(NumAShrInst);

    JSONwriter->String("conversion_instructions");
    JSONwriter->Uint64(ConversionInst);
    JSONwriter->String("conversion_trunc_instructions");
    JSONwriter->Uint64(NumTruncInst);
    JSONwriter->String("conversion_zext_instructions");
    JSONwriter->Uint64(NumZExtInst);
    JSONwriter->String("conversion_sext_instructions");
    JSONwriter->Uint64(NumSExtInst);
    JSONwriter->String("conversion_fptrunc_instructions");
    JSONwriter->Uint64(NumFPTruncInst);
    JSONwriter->String("conversion_fpext_instructions");
    JSONwriter->Uint64(NumFPExtInst);
    JSONwriter->String("conversion_fptoui_instructions");
    JSONwriter->Uint64(NumFPToUIInst);
    JSONwriter->String("conversion_fptosi_instructions");
    JSONwriter->Uint64(NumFPToSIInst);
    JSONwriter->String("conversion_uitofp_instructions");
    JSONwriter->Uint64(NumUIToFPInst);
    JSONwriter->String("conversion_sitofp_instructions");
    JSONwriter->Uint64(NumSIToFPInst);
    JSONwriter->String("conversion_inttoptr_instructions");
    JSONwriter->Uint64(NumIntToPtrInst);
    JSONwriter->String("conversion_ptrtoint_instructions");
    JSONwriter->Uint64(NumPtrToIntInst);
    JSONwriter->String("conversion_bitcast_instructions");
    JSONwriter->Uint64(NumBitCastInst);
    JSONwriter->String("conversion_address_space_cast_instructions");
    JSONwriter->Uint64(NumAddrSpaceCastInst);

    JSONwriter->String("address_arith_instructions");
    JSONwriter->Uint64(NumAllocaInst + NumGetElementPtrInst);
    JSONwriter->String("address_arith_alloca_arith_instructions");
    JSONwriter->Uint64(NumAllocaInst);
    JSONwriter->String("address_arith_get_elem_ptr_arith_instructions");
    JSONwriter->Uint64(NumGetElementPtrInst);

    JSONwriter->String("fp_arith_instructions");
    JSONwriter->Uint64(FPInst);
    JSONwriter->String("fp_arith_add_instructions");
    JSONwriter->Uint64(FAddInst);
    JSONwriter->String("fp_arith_sub_instructions");
    JSONwriter->Uint64(FSubInst);
    JSONwriter->String("fp_arith_mul_instructions");
    JSONwriter->Uint64(FMulInst);
    JSONwriter->String("fp_arith_div_instructions");
    JSONwriter->Uint64(FDivInst);
    JSONwriter->String("fp_arith_rem_instructions");
    JSONwriter->Uint64(FRemInst);

    JSONwriter->String("fp_cmp_instructions");
    JSONwriter->Uint64(NumFCmpInst);

    JSONwriter->String("control_instructions");
    JSONwriter->Uint64(CtrlInst);
    JSONwriter->String("control_call_instructions");
    JSONwriter->Uint64(NumCallInst);
    JSONwriter->String("control_ret_instructions");
    JSONwriter->Uint64(NumRetInst);
    JSONwriter->String("control_branch_instructions");
    JSONwriter->Uint64(NumBrInst);
    JSONwriter->String("control_switch_instructions");
    JSONwriter->Uint64(NumSwitchInst);
    JSONwriter->String("control_indirectBr_instructions");
    JSONwriter->Uint64(NumIndirectBrInst);
    JSONwriter->String("control_invoke_instructions");
    JSONwriter->Uint64(NumInvokeInst);
    JSONwriter->String("control_resume_instructions");
    JSONwriter->Uint64(NumResumeInst);
    JSONwriter->String("control_unreachable_instructions");
    JSONwriter->Uint64(NumUnreachableInst);

    JSONwriter->String("phi_instructions");
    JSONwriter->Uint64(NumPHIInst);

    JSONwriter->String("sync_instructions");
    JSONwriter->Uint64(NumFenceInst);
    JSONwriter->String("sync_fence_instructions");
    JSONwriter->Uint64(NumFenceInst);

    JSONwriter->String("vector_instructions");
    JSONwriter->Uint64(NumExtractElementInst + NumInsertElementInst + NumShuffleVectorInst);
    JSONwriter->String("vector_extract_element_instructions");
    JSONwriter->Uint64(NumExtractElementInst);
    JSONwriter->String("vector_insert_element_instructions");
    JSONwriter->Uint64(NumInsertElementInst);
    JSONwriter->String("vector_shuffle_vector_instructions");
    JSONwriter->Uint64(NumShuffleVectorInst);

    JSONwriter->String("aggregate_nonvector_instructions");
    JSONwriter->Uint64(NumExtractValueInst + NumInsertValueInst);
    JSONwriter->String("aggregate_extract_value_instructions");
    JSONwriter->Uint64(NumExtractValueInst);
    JSONwriter->String("aggregate_insert_value_instructions");
    JSONwriter->Uint64(NumInsertValueInst);

    JSONwriter->String("misc_instructions");
    JSONwriter->Uint64(NumSelectInst + NumLandingPadInst + NumVAArgInst);
    JSONwriter->String("misc_select_instructions");
    JSONwriter->Uint64(NumSelectInst);
    JSONwriter->String("misc_landingpad_instructions");
    JSONwriter->Uint64(NumLandingPadInst);
    JSONwriter->String("misc_va_arg_instructions");
    JSONwriter->Uint64(NumVAArgInst);
    // JSONwriter->Uint64(MiscInst + ConversionInst + BitwiseInst);

    JSONwriter->EndObject();
}

std::ostream& operator<<(std::ostream& out, const map<unsigned, map<int, map<int, map<unsigned, unsigned long long> > > >& m) {
    for (const auto& kv1:m) {
       for (const auto& kv2:kv1.second) {
           for (const auto& kv3:kv2.second) {
               for (const auto& kv4:kv3.second) {
                   out << kv1.first
                       << "," << kv2.first
                       << "," << kv3.first
                       << "," << kv4.first
                       << "," << kv4.second
                       << endl;
               }
           }
       }
    }
    return out;
}

void InstructionMix::JSONdump_partial(JSONmanager *JSONwriter, 
                                      unsigned inst_type,
                                      int scalar_type, 
                                      int vector_size, 
                                      unsigned long long *sum_partial, 
                                      unsigned isFloat) {

    if (isFloat == 1 || isFloat == 2) {
        JSONwriter->String("float_16bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::HalfTyID]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::HalfTyID];
        JSONwriter->String("float_32bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::FloatTyID]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::FloatTyID];
        JSONwriter->String("float_64bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::DoubleTyID]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::DoubleTyID];
        JSONwriter->String("float_80bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::X86_FP80TyID]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::X86_FP80TyID];
        JSONwriter->String("float_128bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::FP128TyID]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::FP128TyID];
        JSONwriter->String("float_128bits_powerpc");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::PPC_FP128TyID]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::PPC_FP128TyID];
        JSONwriter->String("float_64bits_mmx");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::X86_MMXTyID]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::X86_MMXTyID];
    }

    if (isFloat == 0 || isFloat == 2) {
        JSONwriter->String("int_4bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][INT_4BITS]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][INT_4BITS];
        JSONwriter->String("int_8bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][INT_8BITS]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][INT_8BITS];
        JSONwriter->String("int_16bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][INT_16BITS]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][INT_16BITS];
        JSONwriter->String("int_32bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][INT_32BITS]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][INT_32BITS];
        JSONwriter->String("int_64bits");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][INT_64BITS]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][INT_64BITS];
        JSONwriter->String("int_misc");
        JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][INT_MISCBITS]);
        (*sum_partial) += counters[inst_type][scalar_type][vector_size][INT_MISCBITS];

        /* TODO */
        // JSONwriter->String("misc");
        // JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][INT_MISCBITS]);
        // (*sum_partial) += counters[inst_type][scalar_type][vector_size][INT_MISCBITS];
    }
    
    JSONwriter->String("struct_type");
    JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::StructTyID]);
    (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::StructTyID];
    JSONwriter->String("array_type");
    JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::ArrayTyID]);
    (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::ArrayTyID];
    JSONwriter->String("pointer_type");
    JSONwriter->Uint64(counters[inst_type][scalar_type][vector_size][Type::PointerTyID]);
    (*sum_partial) += counters[inst_type][scalar_type][vector_size][Type::PointerTyID];

    return;
}

void InstructionMix::JSONdump_perType(JSONmanager *JSONwriter,
                                      char* inst_type_name, 
                                      unsigned inst_type_LLVM, 
                                      unsigned long long inst, 
                                      unsigned isFloat) {
    unsigned long long scalarInst = 0, vectorInst = 0;

    JSONwriter -> StartArray();
    JSONwriter -> StartObject();


    std::ostringstream header, scalarHeader, vectorHeader, miscHeader;
    header << inst_type_name << "_mix";
    
    scalarHeader << "scalar_instructions";
    vectorHeader << "vector_instructions";
    miscHeader << "misc_instructions";

    JSONwriter->String("total_instructions");
    JSONwriter->Uint64(inst);
    JSONwriter->String("mix");
        JSONwriter->StartArray();

            JSONwriter->StartObject();
            JSONwriter->String("vector_size");
            JSONwriter->Uint64(1);
            JSONdump_partial(JSONwriter, inst_type_LLVM, TY_SCALAR, VECTOR_SIZE_SCALAR, &scalarInst, isFloat);
            JSONwriter->String("instructions");
            JSONwriter->Uint64(scalarInst);
            JSONwriter->EndObject();

            for(auto kv1:counters[inst_type_LLVM][TY_VECTOR]) {
                JSONwriter->StartObject();
                    unsigned long long sum_partial = 0;
                    JSONwriter->String("vector_size");
                    int vectorSize = kv1.first;
                    JSONwriter->Uint64(vectorSize);
                    JSONdump_partial(JSONwriter, inst_type_LLVM, TY_VECTOR, vectorSize, &sum_partial, isFloat);
                    JSONwriter->String("instructions");
                    JSONwriter->Uint64(sum_partial);
                    vectorInst+=sum_partial;
                JSONwriter->EndObject();
            }

        JSONwriter->EndArray();
    JSONwriter->String(scalarHeader.str().c_str());
    JSONwriter->Uint64(scalarInst);
    JSONwriter->String(vectorHeader.str().c_str());
    JSONwriter->Uint64(vectorInst);
    JSONwriter->String(miscHeader.str().c_str());
    JSONwriter->Uint64(counters[inst_type_LLVM][TY_MISC][VECTOR_SIZE_MISC][0]);

    JSONwriter -> EndObject();
    JSONwriter -> EndArray();
    
    return;
}

void InstructionMix::JSONdump_vectorIncluded(JSONmanager *JSONwriter) {
    unsigned long long CtrlInst         = getNumCtrlInst();
    unsigned long long IntInst          = getNumIntInst();
    unsigned long long FPInst           = getNumFPInst();
    unsigned long long FAddInst         = getNumFAddInst();
    unsigned long long FSubInst         = getNumFSubInst();
    unsigned long long FMulInst         = getNumFMulInst();
    unsigned long long FDivInst         = getNumFDivInst();
    unsigned long long FRemInst         = getNumFRemInst();
    unsigned long long IntAddInst       = getNumIntAddInst();
    unsigned long long IntSubInst       = getNumIntSubInst();
    unsigned long long IntMulInst       = getNumIntMulInst();
    unsigned long long IntUDivInst      = getNumIntUDivInst();
    unsigned long long IntSDivInst      = getNumIntSDivInst();
    unsigned long long IntURemInst      = getNumIntURemInst();
    unsigned long long IntSRemInst      = getNumIntSRemInst();
    unsigned long long MemInst          = getNumMemInst();
    unsigned long long LoadInst         = getNumLoadInst();
    unsigned long long StoreInst        = getNumStoreInst();
    unsigned long long BitwiseInst      = getNumBitwiseInst();
    unsigned long long ConversionInst   = getNumConversionInst();
    unsigned long long MiscInst         = getNumMiscInst();

    JSONwriter->String("instructionMix");
    JSONwriter->StartObject();

    JSONwriter->String("instructions_analyzed");
    JSONwriter->Uint64(NumTotalInsts);

    JSONwriter->String("load_instructions");
    JSONdump_perType(JSONwriter, (char*)"load", Instruction::Load, LoadInst, PRINT_FLOATS_INTS);

    JSONwriter->String("store_instructions");
    JSONdump_perType(JSONwriter, (char*)"store" , Instruction::Store, StoreInst, PRINT_FLOATS_INTS);

    /* These instructions are not breakdown to scalar and vector instructions (they are 0 for all tested benchmarks). */
    JSONwriter->String("atomic_memory_instructions");
    JSONwriter->Uint64(NumAtomicCmpXchgInst + NumAtomicRMWInst);
    JSONwriter->String("atomic_memory_cmpxchg_instructions");
    JSONwriter->Uint64(NumAtomicCmpXchgInst);
    JSONwriter->String("atomic_memory_rmw_instructions");
    JSONwriter->Uint64(NumAtomicRMWInst);

    JSONwriter->String("int_arith_instructions");
    JSONwriter->Uint64(IntInst);

    JSONwriter->String("int_arith_add_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_arith_add" , Instruction::Add, IntAddInst, PRINT_ONLY_INTS);

    JSONwriter->String("int_arith_sub_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_arith_sub" , Instruction::Sub, IntSubInst, PRINT_ONLY_INTS);

    JSONwriter->String("int_arith_mul_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_arith_sub" , Instruction::Mul, IntMulInst, PRINT_ONLY_INTS);

    JSONwriter->String("int_arith_udiv_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_arith_udiv" , Instruction::UDiv, IntUDivInst, PRINT_ONLY_INTS);

    JSONwriter->String("int_arith_sdiv_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_arith_sdiv" , Instruction::SDiv, IntSDivInst, PRINT_ONLY_INTS);

    JSONwriter->String("int_arith_urem_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_arith_urem" , Instruction::URem, IntURemInst, PRINT_ONLY_INTS);

    JSONwriter->String("int_arith_srem_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_arith_srem" , Instruction::SRem, IntSRemInst, PRINT_ONLY_INTS);

    JSONwriter->String("int_cmp_instructions");
    JSONdump_perType(JSONwriter, (char*)"int_cmp" , Instruction::ICmp, NumICmpInst, PRINT_ONLY_INTS);

    JSONwriter->String("fp_arith_instructions");
    JSONwriter->Uint64(FPInst);

    JSONwriter->String("fp_arith_add_instructions");
    JSONdump_perType(JSONwriter, (char*)"fp_arith_add" , Instruction::FAdd, FAddInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("fp_arith_sub_instructions");
    JSONdump_perType(JSONwriter, (char*)"fp_arith_sub" , Instruction::FSub, FSubInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("fp_arith_mul_instructions");
    JSONdump_perType(JSONwriter, (char*)"fp_arith_mul" , Instruction::FMul, FMulInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("fp_arith_div_instructions");
    JSONdump_perType(JSONwriter, (char*)"fp_arith_sdiv" , Instruction::FDiv, FDivInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("fp_arith_rem_instructions");
    JSONdump_perType(JSONwriter, (char*)"fp_arith_rem" , Instruction::FRem, FRemInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("fp_cmp_instructions");
    JSONdump_perType(JSONwriter, (char*)"fp_cmp" , Instruction::FCmp, NumFCmpInst, PRINT_ONLY_FLOATS);
    
        
    JSONwriter->String("bitwise_instructions");
    JSONwriter->Uint64(BitwiseInst);
    
    JSONwriter->String("bitwise_and_instructions");
    JSONdump_perType(JSONwriter, (char*)"bitwise_and" , Instruction::And, NumAndInst, PRINT_FLOATS_INTS);
    
    JSONwriter->String("bitwise_or_instructions");
    JSONdump_perType(JSONwriter, (char*)"bitwise_or" , Instruction::Or, NumOrInst, PRINT_FLOATS_INTS);
    
    JSONwriter->String("bitwise_xor_instructions");
    JSONdump_perType(JSONwriter, (char*)"bitwise_xor" , Instruction::Xor, NumXorInst, PRINT_FLOATS_INTS);
    
    JSONwriter->String("bitwise_shift_left_instructions");
    JSONdump_perType(JSONwriter, (char*)"bitwise_shift_left" , Instruction::Shl, NumShlInst, PRINT_FLOATS_INTS);
    
    JSONwriter->String("bitwise_logical_shift_right_instructions");
    JSONdump_perType(JSONwriter, (char*)"bitwise_logical_shift_right" , Instruction::LShr, NumLShrInst, PRINT_FLOATS_INTS);
    
    JSONwriter->String("bitwise_arith_shift_right_instructions");
    JSONdump_perType(JSONwriter, (char*)"bitwise_arith_shift_right" , Instruction::AShr, NumAShrInst, PRINT_FLOATS_INTS);
    

    JSONwriter->String("conversion_instructions");
    JSONwriter->Uint64(ConversionInst);

    JSONwriter->String("conversion_trunc_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_trunc" , Instruction::Trunc, NumTruncInst, PRINT_ONLY_INTS);

    JSONwriter->String("conversion_zext_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_zext" , Instruction::ZExt, NumZExtInst, PRINT_ONLY_INTS);

    JSONwriter->String("conversion_sext_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_sext" , Instruction::SExt, NumSExtInst, PRINT_ONLY_INTS);

    JSONwriter->String("conversion_fptrunc_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_fptrunc" , Instruction::FPTrunc, NumFPTruncInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("conversion_fpext_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_fpext" , Instruction::FPExt, NumFPExtInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("conversion_fptoui_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_fptoui" , Instruction::FPToUI, NumFPToUIInst, PRINT_ONLY_INTS);

    JSONwriter->String("conversion_fptosi_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_fptosi" , Instruction::FPToSI, NumFPToSIInst, PRINT_ONLY_INTS);

    JSONwriter->String("conversion_uitofp_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_uitofp" , Instruction::UIToFP, NumUIToFPInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("conversion_sitofp_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_sitofp" , Instruction::SIToFP, NumSIToFPInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("conversion_inttoptr_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_inttoptr" , Instruction::IntToPtr, NumIntToPtrInst, PRINT_FLOATS_INTS);

    JSONwriter->String("conversion_ptrtoint_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_ptrtoint" , Instruction::PtrToInt, NumPtrToIntInst, PRINT_ONLY_FLOATS);

    JSONwriter->String("conversion_bitcast_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_bitcast" , Instruction::BitCast, NumBitCastInst, PRINT_FLOATS_INTS);

    JSONwriter->String("conversion_address_space_cast_instructions");
    JSONdump_perType(JSONwriter, (char*)"conversion_address_space_cast" , Instruction::AddrSpaceCast, NumAddrSpaceCastInst, PRINT_FLOATS_INTS);

    JSONwriter->String("address_arith_instructions");
    JSONwriter->Uint64(NumAllocaInst + NumGetElementPtrInst);

    JSONwriter->String("address_arith_alloca_arith_instructions");
    JSONwriter->Uint64(NumAllocaInst);

    JSONwriter->String("address_arith_get_elem_ptr_arith_instructions");
    JSONdump_perType(JSONwriter, (char*)"address_arith_get_elem_ptr_arith" , Instruction::GetElementPtr, NumGetElementPtrInst, PRINT_FLOATS_INTS);

    JSONwriter->String("control_instructions");
    JSONwriter->Uint64(CtrlInst);
    JSONwriter->String("control_call_instructions");
    JSONwriter->Uint64(NumCallInst);
    JSONwriter->String("control_ret_instructions");
    JSONwriter->Uint64(NumRetInst);
    JSONwriter->String("control_branch_instructions");
    JSONwriter->Uint64(NumBrInst);
    JSONwriter->String("control_switch_instructions");
    JSONwriter->Uint64(NumSwitchInst);
    JSONwriter->String("control_indirectBr_instructions");
    JSONwriter->Uint64(NumIndirectBrInst);
    JSONwriter->String("control_invoke_instructions");
    JSONwriter->Uint64(NumInvokeInst);
    JSONwriter->String("control_resume_instructions");
    JSONwriter->Uint64(NumResumeInst);
    JSONwriter->String("control_unreachable_instructions");
    JSONwriter->Uint64(NumUnreachableInst);

    JSONwriter->String("phi_instructions");
    JSONwriter->Uint64(NumPHIInst);

    JSONwriter->String("sync_instructions");
    JSONwriter->Uint64(NumFenceInst);
    JSONwriter->String("sync_fence_instructions");
    JSONwriter->Uint64(NumFenceInst);

    JSONwriter->String("llvm_vector_instructions");
    JSONwriter->Uint64(NumExtractElementInst + NumInsertElementInst + NumShuffleVectorInst);

    JSONwriter->String("llvm_vector_extract_element_instructions");
    JSONdump_perType(JSONwriter, (char*)"llvm_vector_extract_element" , Instruction::ExtractElement, NumExtractElementInst, PRINT_FLOATS_INTS);

    JSONwriter->String("llvm_vector_insert_element_instructions");
    JSONdump_perType(JSONwriter, (char*)"llvm_vector_insert_element" , Instruction::InsertElement, NumInsertElementInst, PRINT_FLOATS_INTS);

    JSONwriter->String("llvm_vector_shuffle_vector_instructions");
    JSONdump_perType(JSONwriter, (char*)"llvm_vector_shuffle_vector" , Instruction::ShuffleVector, NumShuffleVectorInst, PRINT_FLOATS_INTS);

    JSONwriter->String("aggregate_nonvector_instructions");
    JSONwriter->Uint64(NumExtractValueInst + NumInsertValueInst);

    JSONwriter->String("aggregate_extract_value_instructions");
    JSONdump_perType(JSONwriter, (char*)"aggregate_extract_value" , Instruction::ExtractValue, NumExtractValueInst, PRINT_FLOATS_INTS);

    JSONwriter->String("aggregate_insert_value_instructions");
    JSONdump_perType(JSONwriter, (char*)"aggregate_insert_value" , Instruction::InsertValue, NumInsertValueInst, PRINT_FLOATS_INTS);

    JSONwriter->String("misc_instructions");
    JSONwriter->Uint64(NumSelectInst + NumLandingPadInst + NumVAArgInst);
    JSONwriter->String("misc_select_instructions");
    JSONdump_perType(JSONwriter, (char*)"misc_select" , Instruction::Select, NumSelectInst, PRINT_FLOATS_INTS);
    JSONwriter->String("misc_landingpad_instructions");
    JSONwriter->Uint64(NumLandingPadInst);
    JSONwriter->String("misc_va_arg_instructions");
    JSONwriter->Uint64(NumVAArgInst);

    JSONwriter->EndObject();
}


void InstructionMix::visitMemoryInstruction(Instruction &I) {
    unsigned long long OpCode = I.getOpcode();
    IntegerType *IntegerTy;
    VectorType *VectorTy;
    raw_os_ostream myCout(cout);

    // Update memory accesses counters
    if (OpCode == Instruction::Load) {
        Type * Ty = I.getOperand(0)->getType();

        if (PointerType * PT = dyn_cast<PointerType>(Ty)) {
            // Should always be true

            unsigned typeID = PT->getElementType()->getTypeID();
            // cout << " I.getOperand(0)->getElementType()->getTypeID()=" << typeID << " + " << Ty->getScalarType() << endl;
            // cout << " address=" << getMemoryAddress(&I) << " value=" << *((float*)getMemoryAddress(&I)) << endl;

            switch (typeID) {

                // Scalar instructions
                case Type::HalfTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumHalfTyLoadInst++;
                    break;
                case Type::FloatTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumFloatTyLoadInst++;
                    break;
                case Type::DoubleTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumDoubleTyLoadInst++;
                    break;
                case Type::X86_FP80TyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumX86_FP80TyLoadInst++;
                    break;
                case Type::FP128TyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumFP128TyLoadInst++;
                    break;
                case Type::PPC_FP128TyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumPPC_FP128TyLoadInst++;
                    break;
                case Type::X86_MMXTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumX86_MMXTyLoadInst++;
                    break;
                case Type::IntegerTyID:
                    IntegerTy = dyn_cast<IntegerType>(PT->getElementType());
                    switch (IntegerTy->getBitWidth()) {
                        case 4:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_4BITS]++;
                            Num4BitsLoadIntInst++;
                            break;
                        case 8:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_8BITS]++;
                            Num8BitsLoadIntInst++;
                            break;
                        case 16:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_16BITS]++;
                            Num16BitsLoadIntInst++;
                            break;
                        case 32:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_32BITS]++;
                            Num32BitsLoadIntInst++;
                            break;
                        case 64:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_64BITS]++;
                            Num64BitsLoadIntInst++;
                            break;
                        default:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_MISCBITS]++;
                            break;
                    }
                    break;

                // Vector instructions
                case Type::VectorTyID: {
                    VectorTy = dyn_cast<VectorType>(PT->getElementType());
                    unsigned vectorOperandType = VectorTy->getElementType()->getTypeID();
                    unsigned vectSize = VectorTy->getNumElements();

                    // cout << " Opcode = " << OpCode << " Vector operand type =" << vectorOperandType << " Vector size = " << VectorTy->getNumElements() << endl;

                    if ( (vectorOperandType <= 6) || (vectorOperandType == 9) )
                        counters[OpCode][TY_VECTOR][vectSize][vectorOperandType]++;
                    else if (vectorOperandType == Type::IntegerTyID) {
                        IntegerTy = dyn_cast<IntegerType>(VectorTy->getElementType());
                        switch (IntegerTy->getBitWidth()) {
                            case 4:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_4BITS]++;
                                    Num4BitsLoadIntInst++;
                                    break;
                            case 8:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_8BITS]++;
                                    Num8BitsLoadIntInst++;
                                    break;
                            case 16:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_16BITS]++;
                                    Num16BitsLoadIntInst++;
                                    break;
                            case 32:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_32BITS]++;
                                    Num32BitsLoadIntInst++;
                                    break;
                            case 64:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_64BITS]++;
                                    Num64BitsLoadIntInst++;
                                    break;
                            default:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_MISCBITS]++;
                                    break;
                        }
                    } else {
                        counters[OpCode][TY_VECTOR][vectSize][vectorOperandType]++;
                    }
                    break;
                }
                
                // Scalar struct instructions
                case Type::StructTyID: {
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    break;
                }
                // Scalar array instructions
                case Type::ArrayTyID: {
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    break;
                }
                // Scalar pointer instructions
                case Type::PointerTyID: {
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    break;
                }
                

                default: {
                    counters[OpCode][TY_MISC][VECTOR_SIZE_MISC][0]++;
                    NumMiscTyLoadInst++;
                    break;
                }
            }
        }
    }

    // Update memory accesses counters
    if (OpCode == Instruction::Store) {
        Type * Ty = I.getOperand(1)->getType();
        //I.print(myCout); cout << "\n";

        if (PointerType * PT = dyn_cast<PointerType>(Ty)) {
            // Should always be true
            unsigned typeID = PT->getElementType()->getTypeID();
            // cout << " I.getOperand(1)->getTypeID()=" << typeID << " + " << PT->getElementType()->getScalarType() << endl;

            switch (typeID) {
                case Type::HalfTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumHalfTyStoreInst++;
                    break;
                case Type::FloatTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumFloatTyStoreInst++;
                    break;
                case Type::DoubleTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumDoubleTyStoreInst++;
                    break;
                case Type::X86_FP80TyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumX86_FP80TyStoreInst++;
                    break;
                case Type::FP128TyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumFP128TyStoreInst++;
                    break;
                case Type::PPC_FP128TyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumPPC_FP128TyStoreInst++;
                    break;
                case Type::X86_MMXTyID:
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    NumX86_MMXTyStoreInst++;
                    break;
                case Type::IntegerTyID:
                    IntegerTy = dyn_cast<IntegerType>(PT->getElementType());
                    switch (IntegerTy->getBitWidth()) {
                        case 4:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_4BITS]++;
                            Num4BitsStoreIntInst++;
                            break;
                        case 8:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_8BITS]++;
                            Num8BitsStoreIntInst++;
                            break;
                        case 16:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_16BITS]++;
                            Num16BitsStoreIntInst++;
                            break;
                        case 32:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_32BITS]++;
                            Num32BitsStoreIntInst++;
                            break;
                        case 64:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_64BITS]++;
                            Num64BitsStoreIntInst++;
                            break;
                        default:
                            counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_MISCBITS]++;
                            break;
                    }
                    break;
                // Vector instructions
                case Type::VectorTyID: {
                    VectorTy = dyn_cast<VectorType>(PT->getElementType());
                    unsigned vectorOperandType = VectorTy->getElementType()->getTypeID();
                    unsigned vectSize = VectorTy->getNumElements();

                    // cout << " Opcode = " << OpCode << " Vector operand type =" << vectorOperandType << " Vector size = " << VectorTy->getNumElements() << endl;

                    /* HalfTyID, FloatTyID, DoubleTyID, X86_FP80TyID, FP128TyID, PPC_FP128TyID, X86_MMXTyID */
                    if ((vectorOperandType <= 6) || (vectorOperandType == 9))
                        counters[OpCode][TY_VECTOR][vectSize][vectorOperandType]++;
                    else if (vectorOperandType == Type::IntegerTyID) {
                        IntegerTy = dyn_cast<IntegerType>(VectorTy->getElementType());
                        switch (IntegerTy->getBitWidth()) {
                            case 4:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_4BITS]++;
                                    Num4BitsLoadIntInst++;
                                    break;
                            case 8:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_8BITS]++;
                                    Num8BitsLoadIntInst++;
                                    break;
                            case 16:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_16BITS]++;
                                    Num16BitsLoadIntInst++;
                                    break;
                            case 32:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_32BITS]++;
                                    Num32BitsLoadIntInst++;
                                    break;
                            case 64:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_64BITS]++;
                                    Num64BitsLoadIntInst++;
                                    break;
                            default:
                                    counters[OpCode][TY_VECTOR][vectSize][INT_MISCBITS]++;
                                    break;
                        }
                    } else {
                        counters[OpCode][TY_VECTOR][vectSize][vectorOperandType]++;
                    }
                    break;
                }

                // Scalar struct instructions
                case Type::StructTyID: {
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    break;
                }
                // Scalar array instructions
                case Type::ArrayTyID: {
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    break;
                }
                // Scalar pointer instructions
                case Type::PointerTyID: {
                    counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][typeID]++;
                    break;
                }                
                   

                default:
                    counters[OpCode][TY_MISC][VECTOR_SIZE_MISC][0]++;
                    NumMiscTyStoreInst++;
                    break;
            }
        }
    }
}

// Binary integer and floating-point and Bitwise instructions
void InstructionMix::visitBinaryArithBitwiseCmpConvertSelectInstruction(Instruction &I) {
    unsigned long long OpCode = I.getOpcode();
    IntegerType *IntegerTy;
    VectorType *VectorTy;
    raw_os_ostream myCout(cout);
    Type* type;

    if  ( (OpCode == Instruction::Add)  || (OpCode == Instruction::Mul)  ||
          (OpCode == Instruction::UDiv) || (OpCode == Instruction::Sub)  ||
          (OpCode == Instruction::SDiv) || (OpCode == Instruction::URem) ||
          (OpCode == Instruction::SRem) ||
          (OpCode == Instruction::FAdd) || (OpCode == Instruction::FSub) ||
          (OpCode == Instruction::FMul) || (OpCode == Instruction::FDiv) ||
          (OpCode == Instruction::FRem) ||
          (OpCode == Instruction::And)  || (OpCode == Instruction::Or)   ||
          (OpCode == Instruction::Xor)  || (OpCode == Instruction::Shl)  ||
          (OpCode == Instruction::LShr) || (OpCode == Instruction::AShr) )
          {
              type = I.getOperand(1)->getType();
          }
    else if ( (OpCode == Instruction::ICmp) || (OpCode == Instruction::FCmp) )
        {
            type = I.getOperand(0)->getType();
        }
    else if (OpCode == Instruction::Select)
        {
            type = I.getOperand(1)->getType();
        }
    else if (getInstType(I) == CONV_TYPE)
        {
            type = I.getType();
        }
    else if (OpCode == Instruction::GetElementPtr)
        {
            // type = I.getType();
            // I.print(myCout); cout << "\n";
            // type->dump(); cout << "\n";
            // I.getOperand(0)->getType()->dump(); cout << "\n";
            // I.getOperand(1)->getType()->dump(); cout << "\n";

            if ((I.getNumOperands() == 3) && (I.getOperand(2)->getType()->isVectorTy())) {
                type = I.getOperand(2)->getType();
                // cout << "FOUND getElementPtr instruction that calculates vector of addresses" << "\n";
                VectorTy = dyn_cast<VectorType>(type);
                unsigned vectorOperandType = VectorTy->getElementType()->getTypeID();
                if ((vectorOperandType <= 6) || (vectorOperandType == 9))
                    counters[OpCode][TY_VECTOR][VectorTy->getNumElements()][vectorOperandType]++;
                else if (vectorOperandType == Type::IntegerTyID) {
                    IntegerTy = dyn_cast<IntegerType>(VectorTy->getElementType());
                    unsigned vectSize = VectorTy->getNumElements();
                    switch (IntegerTy->getBitWidth()) {
                        case 4:
                                counters[OpCode][TY_VECTOR][vectSize][INT_4BITS]++;
                                break;
                        case 8:
                                counters[OpCode][TY_VECTOR][vectSize][INT_8BITS]++;
                                break;
                        case 16:
                                counters[OpCode][TY_VECTOR][vectSize][INT_16BITS]++;
                                break;
                        case 32:
                                counters[OpCode][TY_VECTOR][vectSize][INT_32BITS]++;
                                break;
                        case 64:
                                counters[OpCode][TY_VECTOR][vectSize][INT_64BITS]++;
                                break;
                        default:
                                counters[OpCode][TY_VECTOR][vectSize][INT_MISCBITS]++;
                                break;
                    }
                } else
                    counters[OpCode][TY_VECTOR][VectorTy->getNumElements()][vectorOperandType]++;
            } else
                counters[OpCode][TY_MISC][VECTOR_SIZE_MISC][0]++;
            return;
        }
    else if (OpCode == Instruction::ExtractElement ||  OpCode == Instruction::InsertElement ||
             OpCode == Instruction::ShuffleVector)
        {
            type = type = I.getOperand(0)->getType();
            // I.print(myCout); cout << "\n";
        }
    else
        return;

    // pthread_mutex_lock(output_lock);
    // I.print(myCout); cout << "\n";
    // type->dump(); cout << "\n";
    // pthread_mutex_unlock(output_lock);

    // Type* type = I.getOperand(1)->getType();
    // I.getOperand(1)->getType()->dump();
    // cout << " getTypeID=" << I.getOperand(1)->getType()->getTypeID();

    if (type->isVoidTy())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][0]++;
    else if (type->isHalfTy())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][1]++;
    else if (type->isFloatTy())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][2]++;
    else if (type->isDoubleTy())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][3]++;
    else if (type->isX86_FP80Ty())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][4]++;
    else if (type->isFP128Ty())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][5]++;
    else if (type->isPPC_FP128Ty())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][6]++;
    else if (type->isX86_MMXTy())
        counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][9]++;
    else if (type->isIntegerTy()) {
        IntegerTy = dyn_cast<IntegerType>(type);
        switch (IntegerTy->getBitWidth()) {
            case 4:
                counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_4BITS]++;
                break;
            case 8:
                counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_8BITS]++;
                break;
            case 16:
                counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_16BITS]++;
                break;
            case 32:
                counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_32BITS]++;
                break;
            case 64:
                counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_64BITS]++;
                break;
            default:
                counters[OpCode][TY_SCALAR][VECTOR_SIZE_SCALAR][INT_MISCBITS]++;
                break;
        }
    } else if (type->isVectorTy()) {
            VectorTy = dyn_cast<VectorType>(type);
            unsigned vectorOperandType = VectorTy->getElementType()->getTypeID();
            if ((vectorOperandType <= 6) || (vectorOperandType == 9))
                counters[OpCode][TY_VECTOR][VectorTy->getNumElements()][vectorOperandType]++;
            else if (vectorOperandType == Type::IntegerTyID) {
                IntegerTy = dyn_cast<IntegerType>(VectorTy->getElementType());
                unsigned vectSize = VectorTy->getNumElements();
                switch (IntegerTy->getBitWidth()) {
                    case 4:
                            counters[OpCode][TY_VECTOR][vectSize][INT_4BITS]++;
                            break;
                    case 8:
                            counters[OpCode][TY_VECTOR][vectSize][INT_8BITS]++;
                            break;
                    case 16:
                            counters[OpCode][TY_VECTOR][vectSize][INT_16BITS]++;
                            break;
                    case 32:
                            counters[OpCode][TY_VECTOR][vectSize][INT_32BITS]++;
                            break;
                    case 64:
                            counters[OpCode][TY_VECTOR][vectSize][INT_64BITS]++;
                            break;
                    default:
                            counters[OpCode][TY_VECTOR][vectSize][INT_MISCBITS]++;
                            break;
                }
            } else {
                counters[OpCode][TY_VECTOR][VectorTy->getNumElements()][vectorOperandType]++;
            }
        } else {
            counters[OpCode][TY_MISC][VECTOR_SIZE_MISC][0]++;
        }
    }
