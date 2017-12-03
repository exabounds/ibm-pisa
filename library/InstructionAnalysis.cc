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

#include "InstructionAnalysis.h"
#include "utils.h"

InstructionAnalysis::InstructionAnalysis(Module *M, int thread_id, int processor_id) :
                                         ld(new DataLayout(M)) {
    this->M = M;
    this->thread_id = thread_id;
    this->processor_id = processor_id;
}

int InstructionAnalysis::getInstType(Instruction &I) {
    unsigned long long OpCode = I.getOpcode();

    switch (OpCode) {
        // Control instructions
        case Instruction::Ret:          return CTRL_TYPE;
        case Instruction::Br:           return CTRL_TYPE;
        case Instruction::Switch:       return CTRL_TYPE;
        case Instruction::IndirectBr:   return CTRL_TYPE;
        case Instruction::Invoke:       return CTRL_TYPE;
        case Instruction::Resume:       return CTRL_TYPE;
        case Instruction::Unreachable:  return CTRL_TYPE;
        case Instruction::Call:         return CTRL_TYPE;

        case Instruction::ICmp:         return INT_CMP_TYPE;
        case Instruction::FCmp:         return FP_CMP_TYPE;
        case Instruction::PHI:          return PHI_TYPE;
        case Instruction::Select:       return MISC_TYPE;

        // Integer operators
        case Instruction::Add:  return INT_ADD_TYPE;
        case Instruction::Sub:  return INT_SUB_TYPE;
        case Instruction::Mul:  return INT_MUL_TYPE;
        case Instruction::UDiv: return INT_UDIV_TYPE;
        case Instruction::SDiv: return INT_SDIV_TYPE;
        case Instruction::URem: return INT_UREM_TYPE;
        case Instruction::SRem: return INT_SREM_TYPE;

        // Floating-point operators
        case Instruction::FAdd: return FP_ADD_TYPE;
        case Instruction::FSub: return FP_SUB_TYPE;
        case Instruction::FMul: return FP_MUL_TYPE;
        case Instruction::FDiv: return FP_DIV_TYPE;
        case Instruction::FRem: return FP_REM_TYPE;

        // Load-Store Instructions
        case Instruction::Load: return getMemoryInstType(I);
        case Instruction::Store:return getMemoryInstType(I);

        // Memory instructions
        case Instruction::Alloca:        return ARITH_ADDRESS_TYPE;
        case Instruction::AtomicCmpXchg: return ATOMIC_MEM_TYPE;
        case Instruction::AtomicRMW:     return ATOMIC_MEM_TYPE;
        case Instruction::Fence:         return SYNC_TYPE;
        case Instruction::GetElementPtr: return ARITH_ADDRESS_TYPE;

        // Bitwise instructions
        case Instruction::And: return BITWISE_TYPE;
        case Instruction::Or : return BITWISE_TYPE;
        case Instruction::Xor: return BITWISE_TYPE;
        case Instruction::Shl: return BITWISE_TYPE;
        case Instruction::LShr:return BITWISE_TYPE;
        case Instruction::AShr:return BITWISE_TYPE;

        // Convert instructions
        case Instruction::Trunc:     return CONV_TYPE;
        case Instruction::ZExt:      return CONV_TYPE;
        case Instruction::SExt:      return CONV_TYPE;
        case Instruction::FPTrunc:   return CONV_TYPE;
        case Instruction::FPExt:     return CONV_TYPE;
        case Instruction::FPToUI:    return CONV_TYPE;
        case Instruction::FPToSI:    return CONV_TYPE;
        case Instruction::UIToFP:    return CONV_TYPE;
        case Instruction::SIToFP:    return CONV_TYPE;
        case Instruction::IntToPtr:  return CONV_TYPE;
        case Instruction::PtrToInt:  return CONV_TYPE;
        case Instruction::BitCast:   return CONV_TYPE;
        case Instruction::AddrSpaceCast: return CONV_TYPE;

        // Misc instructions
        case Instruction::VAArg:          return MISC_TYPE;
        case Instruction::LandingPad:     return MISC_TYPE;

        // Vector instructions...
        case Instruction::ExtractElement: return MISC_VECTOR_TYPE;
        case Instruction::InsertElement:  return MISC_VECTOR_TYPE;
        case Instruction::ShuffleVector:  return MISC_VECTOR_TYPE;

        // Aggregate non-vector instructions
        case Instruction::ExtractValue:   return MISC_AGGREGATE_TYPE;
        case Instruction::InsertValue:    return MISC_AGGREGATE_TYPE;

        default: return -1;
    }
}

bool InstructionAnalysis::isOfIntegerType(Instruction &I) {
    int OpCode = getInstType(I);

    switch (OpCode) { 
        // Conversion instructions
        case CONV_TYPE: return true;

        // Bitwise instructions
        case BITWISE_TYPE: return true;

        // Integer arithmetic instructions
        case INT_ADD_TYPE:  return true;
        case INT_SUB_TYPE:  return true;
        case INT_MUL_TYPE:  return true;
        case INT_UDIV_TYPE: return true;
        case INT_SDIV_TYPE: return true;
        case INT_UREM_TYPE: return true;
        case INT_SREM_TYPE: return true;

        // Integer comparison instructions
        case INT_CMP_TYPE: return true;

        // Integer address arithmetic calculations
        case ARITH_ADDRESS_TYPE: return true;

        default: return false;
    }
}

bool InstructionAnalysis::isOfControlType(Instruction &I) {
    int OpCode = getInstType(I);

    switch (OpCode) {
        // Control instructions
        case CTRL_TYPE: return true;
        default: return false;
    }
}

bool InstructionAnalysis::isOfFloatingPointType(Instruction &I) {
    int OpCode = getInstType(I);

    switch (OpCode) {
        // Floating-point arithmetic instructions
        case FP_ADD_TYPE:         return true;
        case FP_SUB_TYPE:         return true;
        case FP_MUL_TYPE:         return true;
        case FP_DIV_TYPE:         return true;
        case FP_REM_TYPE:         return true;

        // Floating-point comparison instructions
        case FP_CMP_TYPE:         return true;

        default: return false;
    }
}

bool InstructionAnalysis::isOfMemoryType(Instruction &I) {
    int OpType = getInstType(I);
    unsigned long long OpCode = I.getOpcode();

    // Atomic memory instructions
    if (OpType == ATOMIC_MEM_TYPE) return true;

        switch (OpCode) {
            // Load and store instructions
            case Instruction::Load: return true;
            case Instruction::Store:return true;
            default: return false;
        }
}

int InstructionAnalysis::getMemoryInstType (Instruction &I) {
    unsigned long long OpCode = I.getOpcode();
    IntegerType *IntegerTy;

    if (OpCode == Instruction::Load) {
        Type * Ty = I.getOperand(0)->getType();
        if (PointerType * PT = dyn_cast<PointerType>(Ty)) { 
            // Should always be true
            switch (PT->getElementType()->getTypeID()) {
                case Type::HalfTyID:
                    return LD_FP_16BITS_TYPE;
                case Type::FloatTyID:
                    return LD_FP_32BITS_TYPE;
                case Type::DoubleTyID:
                    return LD_FP_64BITS_TYPE;
                case Type::X86_FP80TyID:
                    return LD_FP_80BITS_TYPE;
                case Type::FP128TyID:
                    return LD_FP_128BITS_TYPE;
                case Type::PPC_FP128TyID:
                    return LD_FP_128BITS_TYPE;
                case Type::X86_MMXTyID:
                    return LD_FP_64BITS_TYPE;
                case Type::IntegerTyID:
                    IntegerTy = dyn_cast<IntegerType>(PT->getElementType());
                    switch (IntegerTy->getBitWidth()) {
                        case 4:
                            return LD_INT_4BITS_TYPE;
                        case 8:
                            return LD_INT_8BITS_TYPE;
                        case 16:
                            return LD_INT_16BITS_TYPE;
                        case 32:
                            return LD_INT_32BITS_TYPE;
                        case 64:
                            return LD_INT_64BITS_TYPE;
                        default:
                            return MISC_MEM_TYPE;
                    }
                    break;
                default:
                    return MISC_MEM_TYPE;
            }
        } else {
            // That should never be executed, but if not included we may 
            // get the warning: control may reach end of non-void function
            errs() << "Loading a non-pointer type.\n";
        }
    } else {
        // Update memory accesses counters
        if (OpCode == Instruction::Store) {
            Type * Ty = I.getOperand(1)->getType();
            if (PointerType * PT = dyn_cast<PointerType>(Ty)) {
                // Should always be true
                switch (PT->getElementType()->getTypeID()) {
                    case Type::HalfTyID:
                        return STORE_FP_16BITS_TYPE;
                    case Type::FloatTyID:
                        return STORE_FP_32BITS_TYPE;
                    case Type::DoubleTyID:
                        return STORE_FP_64BITS_TYPE;
                    case Type::X86_FP80TyID:
                        return STORE_FP_80BITS_TYPE;
                    case Type::FP128TyID:
                        return STORE_FP_128BITS_TYPE;
                    case Type::PPC_FP128TyID:
                        return STORE_FP_128BITS_TYPE;
                    case Type::X86_MMXTyID:
                        return STORE_FP_64BITS_TYPE;
                    case Type::IntegerTyID:
                        IntegerTy = dyn_cast<IntegerType>(PT->getElementType());
                        switch (IntegerTy->getBitWidth()) {
                            case 4:
                                return STORE_INT_4BITS_TYPE;
                            case 8:
                                return STORE_INT_8BITS_TYPE;
                            case 16:
                                return STORE_INT_16BITS_TYPE;
                            case 32:
                                return STORE_INT_32BITS_TYPE;
                            case 64:
                                return STORE_INT_64BITS_TYPE;
                            default:
                                return MISC_MEM_TYPE;
                        }
                        break;
                    default:
                        return MISC_MEM_TYPE;
                }
            } else {
                // That should never be executed, but if not included we may 
                // get the warning: control may reach end of non-void function
                errs() << "Trying to load a non-pointer type.\n";
            }
        } else
            errs() << "Not load/store memory instruction\n";
    }

    errs() << "Unknown memory instruction type\n";

    return 0;
}

unsigned InstructionAnalysis::getMemoryAccessSize(Instruction &I) {
    unsigned value = 0;
    unsigned Opcode = I.getOpcode();

    Value *addr = (Opcode == Instruction::Load) ?
        cast<LoadInst>(&I)->getPointerOperand() :
        cast<StoreInst>(&I)->getPointerOperand();

    Type *type = addr->getType();
    if (type->isPointerTy())
        type = type->getContainedType(0);

    value = ld->getTypeAllocSize(type);

    if (!value)
        value = 4; // default

    return value;
}
