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

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

#if LLVM_VERSION_MINOR > 4
#include "llvm/IR/InstIterator.h"
#else
#include "llvm/Support/InstIterator.h"
#endif

#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/StringMap.h"

#include <iostream>
#include <stdio.h>
#include <map>
#include <string>

// Those defines should be the same as in libanalysis.cc
#define BASIC_MPI_CALL  1
#define ASYNC_MPI_CALL  2
#define ANYTAG_MPI_CALL 3
#define SENDRECV_MPI_CALL   4
#define WAIT_MPI_CALL       5
#define TEST_MPI_CALL       6
#define ALL_MPI_CALL        7
#define INIT_MPI_CALL       8
#define FIN_MPI_CALL        9
#define WAITALL_MPI_CALL    10
#define TESTALL_MPI_CALL    11
#define BCAST_MPI_CALL      12
#define IBCAST_MPI_CALL     13
#define ALLTOALL_MPI_CALL   14
#define IALLTOALL_MPI_CALL  15
#define GATHER_MPI_CALL     16
#define IGATHER_MPI_CALL    17
#define ALLGATHER_MPI_CALL  18
#define IALLGATHER_MPI_CALL 19
#define REDUCE_MPI_CALL     20
#define IREDUCE_MPI_CALL    21
#define ALLREDUCE_MPI_CALL  22
#define IALLREDUCE_MPI_CALL 23
#define ALLTOALLV_MPI_CALL  24

using namespace llvm;

// The following global variables are used for adding command line arguments
// to the instrumentation process. Each of them is defining the name of the
// argument, its type and a short description.
cl::opt<std::string> Server("server", cl::desc("Specifies the IP address and the port of the AnalysisServer."), cl::init(""));
cl::opt<std::string> MaxExpectedNrOfThreads("max-expected-threads", cl::desc("Maximum expected number of threads"), cl::init("1"));


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

namespace {
    struct Analysis : public ModulePass {
        static char ID;


        Analysis() : ModulePass(ID) {}

        static void insertInitLibAnalysis(Module &M) {
            Module::iterator F, N;
            Function::arg_iterator mainIter;
            Value* mainArgc;
            Value* mainArgv;
            for (F = M.begin(), N = M.end(); F != N; ++F)
                if (F->getName() == "main"){
                    mainIter = ((Function*)F)->arg_begin();
                    mainArgc = mainIter;
                    mainArgv = ++mainIter;
                    break;
                }

            Function::iterator BB = F->begin();
            BasicBlock::iterator I = BB->begin();

            IRBuilder<> builder(M.getContext());
            builder.SetInsertPoint(BB);

            // insert global string with IP:PORT
            std::vector<Value *> vec;
            const std::string string_ptr = Server + "/" + MaxExpectedNrOfThreads;
            Value * ActualPtrName = builder.CreateGlobalStringPtr(string_ptr.c_str());
            GetElementPtrInst * gepi = GetElementPtrInst::CreateInBounds(ActualPtrName, vec, "", I);

            std::vector<Value *> args;
            args.push_back(gepi);
            args.push_back( mainArgc );
            args.push_back( mainArgv );

            // init_libanalysis(char *) -> (char *ip_port_str)
            Constant *hook = M.getOrInsertFunction("init_libanalysis",
                    Type::getVoidTy(M.getContext()),
                    PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                    mainArgc->getType(),
                    mainArgv->getType(),
                    (Type *)NULL);

            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");
            BB->getInstList().insert(I, new_inst);
            // insertFAddr(M, BB, gepi);
            // insertFAddr(M, BB, new_inst);
            
            // communitates process_ids!
            FunctionType *ftype = FunctionType::get(Type::getVoidTy(M.getContext()), std::vector<Type *>(), true);
            hook = M.getOrInsertFunction("mpi_update_process_id", ftype);

            std::vector<Value *> args2;
            Instruction *new_inst2 = CallInst::Create(cast<Function>(hook), args2, "");
            BB->getInstList().insert(I, new_inst2);
        }


        static void insertFAddr(Module &M, Function::iterator BB, Instruction *I) {
            int f = 0;

            Instruction *new_inst = I;
            for (Module::iterator F = M.begin(), N = M.end(); F != N; ++F, ++f) {
                if (F->isDeclaration())
                    continue;

                std::vector<Type *> argsTy;               
                FunctionType *ftype = FunctionType::get(Type::getVoidTy(M.getContext()), argsTy, true);
                Constant *hook = M.getOrInsertFunction("func_address", ftype);

                std::vector<Value *> args;
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), f));
                args.push_back(F);

                Instruction *prevNewInst = new_inst;

                new_inst = CallInst::Create(cast<Function>(hook), args, "");
                BB->getInstList().insert(prevNewInst, new_inst);
            }
        }



        /* Returns number of variables */
        static void addLSPointerOperand(Module &M, Function::iterator BB, BasicBlock::iterator I, std::vector<Value *> &args) {
            unsigned Opcode;

            Opcode = I->getOpcode();
            Value *addr = (Opcode == Instruction::Load) ?
                cast<LoadInst>(I)->getPointerOperand() :
                cast<StoreInst>(I)->getPointerOperand();

            args.push_back(addr);
        }


        static void insertEndApplication(Module &M, Function::iterator BB, BasicBlock::iterator I) {
            std::vector<Value *> args;
            Constant *hook = M.getOrInsertFunction("end_app", Type::getVoidTy(M.getContext()),
                    (Type *)NULL);
            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");

            BB->getInstList().insert(I, new_inst);
        }


        static void insertLSValues(Module &M, Function::iterator BB, BasicBlock::iterator I,
                int f, int bb, int i) {
            std::vector<Type *> argsTy;

            FunctionType *ftype = FunctionType::get(Type::getVoidTy(M.getContext()), argsTy, true);
            Constant *hook = M.getOrInsertFunction("update_vars", ftype);

            std::vector<Value *> args;

            // Insert IDs
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), f));
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), bb));
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), i));

            addLSPointerOperand(M, BB, I, args);

            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");
            BB->getInstList().insertAfter(I, new_inst);
        }

        static int insertUpdateProcessID(Module &M, Function::iterator BB, BasicBlock::iterator &I,
                int f, int bb, int i) {
            std::vector<Type *> argsTy;


            if (strcmp(I->getOpcodeName(), "call"))
                return 0;

            CallInst *call = cast<CallInst>(I);
            Function *_call = get_calledFunction(call);
            if (!_call || strncasecmp(_call->getName().str().c_str(), "MPI_", 4))
                return 0;


            const char *call_name = _call->getName().str().c_str();
            if (strcasecmp(call_name, "MPI_Init") && strcasecmp(call_name, "MPI_Init_"))
            {
                return 0;
            }
            
            /*
            // ProcessId on the server side has to be initialized at the very beginning otherwise the server does not know which process executes what
            // mpi_update_process_id has to be inserted right after init_libanalysis.
            
                FunctionType *ftype = FunctionType::get(Type::getVoidTy(M.getContext()), argsTy, true);
                Constant * hook = M.getOrInsertFunction("mpi_update_process_id", ftype);

                std::vector<Value *> args;

                Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");
                        BasicBlock::iterator Iold = I;
                        I--;
                        Iold -> dump();
                        BB->getInstList().erase(Iold);
                BB->getInstList().insertAfter(I, new_inst);
            */
            BasicBlock::iterator Iold = I;
            I--;
            BB->getInstList().erase(Iold);

            std::vector<Value *> args2;
            FunctionType *ftype2 = FunctionType::get(Type::getVoidTy(M.getContext()), argsTy, true);

            Constant * hook2;
            hook2 = M.getOrInsertFunction("mpi_update_db", ftype2);
      
            args2.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), f));
                        args2.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), bb));
                        args2.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), i));
            // FORTRAN or C .                       
            if(call_name[strlen(call_name)-1] == '_')
                args2.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 1));
            else
                args2.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 0));
                
            args2.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), INIT_MPI_CALL));
            Instruction *new_inst2 = CallInst::Create(cast<Function>(hook2), args2, "");
                        BB->getInstList().insertAfter(/*new_inst*/I, new_inst2);
                        

            // Instrument the MPI_Init call
            // I++;
            // int ret = insertMPIhooks(M, BB, I, f, bb, i);

            return 1;
        }

        static int insertMPIhooks(Module &M, Function::iterator BB, BasicBlock::iterator &I,
                int f, int bb, int i) {
            std::vector<Type *> argsTy;


            if (strcmp(I->getOpcodeName(), "call"))
                return 0;

            CallInst *call = cast<CallInst>(I);
            Function *_call = get_calledFunction(call);
            if (!_call || strncasecmp(_call->getName().str().c_str(), "MPI_", 4)){
                return 0;
            }

            FunctionType *ftype = FunctionType::get(Type::getVoidTy(M.getContext()), argsTy, true);

            Constant * hook;
            hook = M.getOrInsertFunction("mpi_update_db", ftype);
            

            std::vector<Value *> args;

            // Insert IDs
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), f));
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), bb));
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), i));
            
            // FORTRAN or C .
            // if(_call->getName().str().back() == '_')// This does not compile for LLVM 3.4 becuase we cant use C++11 (or better we would need to solve other errors)
            if(_call->getName().str().c_str()[ strlen( _call->getName().str().c_str() ) -1 ] == '_')
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 1));
            else
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 0));

            const char *call_name = _call->getName().str().c_str();

            if (!strcasecmp(call_name, "MPI_Send") || !strcasecmp(call_name, "MPI_Recv") ||
                    !strcasecmp(call_name, "MPI_Send_") || !strcasecmp(call_name, "MPI_Recv_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), BASIC_MPI_CALL));
                // Insert processor_id of the other end-point communication - 4th argument
                args.push_back(call->getArgOperand(3));
                // Insert the communication tag - 5th argument
                args.push_back(call->getArgOperand(4));
                // Insert the MPI_comm - 6th argument
                args.push_back(call->getArgOperand(5));
                // Insert the number of elements in the buffer
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer
                args.push_back(call->getArgOperand(2));
            } else if (!strcasecmp(call_name, "MPI_Isend") || !strcasecmp(call_name, "MPI_Irecv") ||
                !strcasecmp(call_name, "MPI_Isend_") || !strcasecmp(call_name, "MPI_Irecv_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ASYNC_MPI_CALL));
                // Insert processor_id of the other end-point communication - 4th argument
                args.push_back(call->getArgOperand(3));
                // Insert the communication tag - 5th argument
                args.push_back(call->getArgOperand(4));
                // Insert the MPI_comm - 6th argument
                args.push_back(call->getArgOperand(5));
                // Insert the real memory address of the request - 7th argument
                args.push_back(call->getArgOperand(6));
                // Insert the number of elements in the buffer
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer
                args.push_back(call->getArgOperand(2));
            } else if (!strcasecmp(call_name, "MPI_Wait") || !strcasecmp(call_name, "MPI_Wait_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), WAIT_MPI_CALL));
                // request address
                args.push_back(call->getArgOperand(0));
                // MPI_Wait status
                args.push_back(call->getArgOperand(1));
            } else if (!strcasecmp(call_name, "MPI_Test") || !strcasecmp(call_name, "MPI_Test_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), TEST_MPI_CALL));
                // request address
                args.push_back(call->getArgOperand(0));
                // flag address
                args.push_back(call->getArgOperand(1));
                // MPITest status
                args.push_back(call->getArgOperand(2));
            } else if (!strcasecmp(call_name, "MPI_Waitall") || !strcasecmp(call_name, "MPI_Waitall_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), WAITALL_MPI_CALL));
                //count
                args.push_back(call->getArgOperand(0));
                //array request
                args.push_back(call->getArgOperand(1));
                //array status
                args.push_back(call->getArgOperand(2));
            } else if (!strcasecmp(call_name, "MPI_Testall") || !strcasecmp(call_name, "MPI_Testall_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), TESTALL_MPI_CALL));
                //count
                args.push_back(call->getArgOperand(0));
                //array request
                args.push_back(call->getArgOperand(1));
                // flag address
                args.push_back(call->getArgOperand(2));
                //array status
                args.push_back(call->getArgOperand(3));
            /*
            } else if (!strcasecmp(call_name, "MPI_Waitany") || !strcasecmp(call_name, "MPI_Waitany_")) {
                //count
                args.push_back(call->getArgOperand(0));
                //array request
                args.push_back(call->getArgOperand(1));
                //ret index
                args.push_back(call->getArgOperand(2));
            } else if (!strcasecmp(call_name, "MPI_Waitsome") || !strcasecmp(call_name, "MPI_Waitsome_")) {
                //ocount
                args.push_back(call->getArgOperand(2));
                //array request
                args.push_back(call->getArgOperand(1));
                //array of output indexes
                args.push_back(call->getArgOperand(3));
            */
            } else if (!strcasecmp(call_name, "MPI_Bcast") || !strcasecmp(call_name, "MPI_Bcast_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), BCAST_MPI_CALL));
                // Insert count
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (datatype)
                args.push_back(call->getArgOperand(2));
                // Insert rank of the root process. - 4th argument
                args.push_back(call->getArgOperand(3));
                // Insrt the MPI_comm - 5th argument
                args.push_back(call->getArgOperand(4));
            } else if (!strcasecmp(call_name, "MPI_Ibcast") || !strcasecmp(call_name, "MPI_Ibcast_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), IBCAST_MPI_CALL));
                // Insert count
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (datatype)
                args.push_back(call->getArgOperand(2));
                // Insert rank of the root process - 4th argument
                args.push_back(call->getArgOperand(3));
                // Insrt the MPI_comm - 5th argument
                args.push_back(call->getArgOperand(4));
                // Insert the real memory address of the request - 6th argument
                args.push_back(call->getArgOperand(5));
            } else if (!strcasecmp(call_name, "MPI_Reduce") || !strcasecmp(call_name, "MPI_Reduce_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), REDUCE_MPI_CALL));
                // Insert count
                args.push_back(call->getArgOperand(2));
                // Insert the size of an element in the buffer (datatype)
                args.push_back(call->getArgOperand(3));
                // Insert rank of the root process - 4th argument
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 5th argument
                args.push_back(call->getArgOperand(6));
            } else if (!strcasecmp(call_name, "MPI_Ireduce") || !strcasecmp(call_name, "MPI_Ireduce_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), IREDUCE_MPI_CALL));
                // Insert count
                args.push_back(call->getArgOperand(2));
                // Insert the size of an element in the buffer (datatype)
                args.push_back(call->getArgOperand(3));
                // Insert rank of the root process - 4th argument
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 5th argument
                args.push_back(call->getArgOperand(6));
                // Insert the real memory address of the request - 6th argument
                args.push_back(call->getArgOperand(7));
            } else if (!strcasecmp(call_name, "MPI_Gather") ||	!strcasecmp(call_name, "MPI_Gather_") ) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), GATHER_MPI_CALL));
                // Insert sendcount
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (sendtype)
                args.push_back(call->getArgOperand(2));
                // Insert recvcount
                args.push_back(call->getArgOperand(4));
                // Insert the size of an element in the buffer (recvtype)
                args.push_back(call->getArgOperand(5));
                // Insert rank of the root process - 7th argument
                args.push_back(call->getArgOperand(6));
                // Insrt the MPI_comm - 8th argument
                args.push_back(call->getArgOperand(7));
            } else if (!strcasecmp(call_name, "MPI_Igather") ||	!strcasecmp(call_name, "MPI_Igather_") ) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), IGATHER_MPI_CALL));
                // Insert sendcount
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (sendtype)
                args.push_back(call->getArgOperand(2));
                // Insert recvcount
                args.push_back(call->getArgOperand(4));
                // Insert the size of an element in the buffer (recvtype)
                args.push_back(call->getArgOperand(5));
                // Insert rank of the root process - 7th argument
                args.push_back(call->getArgOperand(6));
                // Insrt the MPI_comm - 8th argument
                args.push_back(call->getArgOperand(7));
                // Insert the real memory address of the request - 9th argument
                args.push_back(call->getArgOperand(8));
            } else if (!strcasecmp(call_name, "MPI_Scatter") ||	!strcasecmp(call_name, "MPI_Scatter_") ) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ANYTAG_MPI_CALL));
                // Rank of the root process - 7th argument
                args.push_back(call->getArgOperand(6));
                // Insrt the MPI_comm - 8th argument
                args.push_back(call->getArgOperand(7));
            } else if (!strcasecmp(call_name, "MPI_Scatterv") || !strcasecmp(call_name, "MPI_Gatherv") ||
                !strcasecmp(call_name, "MPI_Scatterv_") || !strcasecmp(call_name, "MPI_Gatherv_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ANYTAG_MPI_CALL));
                // Rank of the root process - 8th argument
                args.push_back(call->getArgOperand(7));
                // Insrt the MPI_comm - 9th argument
                args.push_back(call->getArgOperand(8));
            } else if (!strcasecmp(call_name, "MPI_Allreduce") || !strcasecmp(call_name, "MPI_Allreduce_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ALLREDUCE_MPI_CALL));
                // Insert count
                args.push_back(call->getArgOperand(2));
                // Insert the size of an element in the buffer (datatype)
                args.push_back(call->getArgOperand(3));
                // Insrt the MPI_comm - 6th argument
                args.push_back(call->getArgOperand(5));
            } else if (!strcasecmp(call_name, "MPI_Iallreduce") || !strcasecmp(call_name, "MPI_Iallreduce_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), IALLREDUCE_MPI_CALL));
                // Insert count
                args.push_back(call->getArgOperand(2));
                // Insert the size of an element in the buffer (datatype)
                args.push_back(call->getArgOperand(3));
                // Insrt the MPI_comm - 6th argument
                args.push_back(call->getArgOperand(5));
                // Insert the real memory address of the request - 7th argument
                args.push_back(call->getArgOperand(6));
            } else if (!strcasecmp(call_name, "MPI_Alltoall") || !strcasecmp(call_name, "MPI_Alltoall_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ALLTOALL_MPI_CALL));
                // Insert sendcount
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (sendtype)
                args.push_back(call->getArgOperand(2));
                // Insert recvcount
                args.push_back(call->getArgOperand(4));
                // Insert the size of an element in the buffer (recvtype)
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 7th argument
                args.push_back(call->getArgOperand(6));
            } else if (!strcasecmp(call_name, "MPI_Alltoallv") || !strcasecmp(call_name, "MPI_Alltoallv_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ALLTOALLV_MPI_CALL));
                // Insert array sendcount
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (sendtype)
                args.push_back(call->getArgOperand(3));
                // Insert array recvcount
                args.push_back(call->getArgOperand(5));
                // Insert the size of an element in the buffer (recvtype)
                args.push_back(call->getArgOperand(7));
                // Insrt the MPI_comm - 7th argument
                args.push_back(call->getArgOperand(8));
            } else if (!strcasecmp(call_name, "MPI_Ialltoall") || !strcasecmp(call_name, "MPI_Ialltoall_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), IALLTOALL_MPI_CALL));
                // Insert sendcount
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (sendtype)
                args.push_back(call->getArgOperand(2));
                // Insert recvcount
                args.push_back(call->getArgOperand(4));
                // Insert the size of an element in the buffer (recvtype)
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 7th argument
                args.push_back(call->getArgOperand(6));
                // Insert the real memory address of the request - 8th argument
                args.push_back(call->getArgOperand(7));
            } else if (!strcasecmp(call_name, "MPI_Allgather") ||	!strcasecmp(call_name, "MPI_Allgather_") ) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ALLGATHER_MPI_CALL));
                // Insert sendcount
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (sendtype)
                args.push_back(call->getArgOperand(2));
                // Insert recvcount
                args.push_back(call->getArgOperand(4));
                // Insert the size of an element in the buffer (recvtype)
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 7th argument
                args.push_back(call->getArgOperand(6));
            } else if (!strcasecmp(call_name, "MPI_Iallgather") ||	!strcasecmp(call_name, "MPI_Iallgather_") ) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), IALLGATHER_MPI_CALL));
                // Insert sendcount
                args.push_back(call->getArgOperand(1));
                // Insert the size of an element in the buffer (sendtype)
                args.push_back(call->getArgOperand(2));
                // Insert recvcount
                args.push_back(call->getArgOperand(4));
                // Insert the size of an element in the buffer (recvtype)
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 7th argument
                args.push_back(call->getArgOperand(6));
                // Insert the real memory address of the request - 8th argument
                args.push_back(call->getArgOperand(7));
            } else if (!strcasecmp(call_name, "MPI_Alltoallv") || !strcasecmp(call_name, "MPI_Alltoallv_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ALL_MPI_CALL));
                // Insrt the MPI_comm - 9th argument
                args.push_back(call->getArgOperand(8));
            } else if (!strcasecmp(call_name, "MPI_Allgatherv") || !strcasecmp(call_name, "MPI_Allgatherv_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), ALL_MPI_CALL));
                // Insrt the MPI_comm - 8th argument
                args.push_back(call->getArgOperand(7));
            } else if (!strcasecmp(call_name, "MPI_Sendrecv") || !strcasecmp(call_name, "MPI_Sendrecv_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), SENDRECV_MPI_CALL));
                // Rank of the destination
                args.push_back(call->getArgOperand(3));
                // Tag for send msg
                args.push_back(call->getArgOperand(4));
                // Rank of the source
                args.push_back(call->getArgOperand(8));
                // Tag for recv msg
                args.push_back(call->getArgOperand(9));
                // Communicator
                args.push_back(call->getArgOperand(10));
            } else if (!strcasecmp(call_name, "MPI_Sendrecv_replace") || !strcasecmp(call_name, "MPI_Sendrecv_replace_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), SENDRECV_MPI_CALL));
                // Rank of the destination
                args.push_back(call->getArgOperand(3));
                // Tag for send msg
                args.push_back(call->getArgOperand(4));
                // Rank of the source
                args.push_back(call->getArgOperand(5));
                // Tag for recv msg
                args.push_back(call->getArgOperand(6));
                // Communicator
                args.push_back(call->getArgOperand(7));
            } else if (!strcasecmp(call_name, "MPI_Init") || !strcasecmp(call_name, "MPI_Init_")) {
                // MPI_init is erased from insertUpdateProcessID
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), INIT_MPI_CALL));
                // Push argc argument
                args.push_back(call->getArgOperand(0));
            } else if (!strcasecmp(call_name, "MPI_Finalize") || !strcasecmp(call_name, "MPI_Finalize_")) {
                // MPI_Init is called from within init_libanalysis. Thus, to support non-MPI application, we need always to MPI_Finalize.
                // Thus we call MPI_Finalize in end_app and in turn we need to remove it from the program if it is there.
                BasicBlock::iterator Iold = I;
                I--;
                BB->getInstList().erase(Iold);
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), FIN_MPI_CALL));
            }

            // Insert request real memory address for async operations.
            // The library will get the rest of the information from the map it holds.
            // The information was previously added by the corresponding Isend/Irecv
            if (!strcasecmp(call_name, "MPI_Isend") || !strcasecmp(call_name, "MPI_Irecv") ||
                !strcasecmp(call_name, "MPI_Isend_") || !strcasecmp(call_name, "MPI_Irecv_")) {
                // Insert the real memory address of the request - 7th argument
                // Indexes start at 0
                args.push_back(call->getArgOperand(6));
            }

            if (args.size() > 4) {
                Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");
                BB->getInstList().insertAfter(I, new_inst);
                return 1;
            }

            return 0;
        }

        static Instruction * insertInstNotification(Module &M, Function::iterator BB, BasicBlock::iterator I,
                int f, int bb, int i, std::string type) {
            std::vector<Value *> args;
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), f));
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), bb));

            Constant *hook = M.getOrInsertFunction(type, Type::getVoidTy(M.getContext()),
                    Type::getInt32Ty(M.getContext()), // Function ID
                    Type::getInt32Ty(M.getContext()), // BB ID
                    (Type *)NULL);
            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");

            BB->getInstList().insert(I, new_inst);
            return new_inst;
        }


        // This is the function run by 'opt' tool for each Module.
        // It's the main function of the instrumentation process.
        virtual bool runOnModule(Module &M) {
            IRBuilder<> builder(M.getContext());

            // This function inserts all the library calls required at the 
            // beginning of the 'main' function.
            // eg: init_libanalysis
            // It also creates the global string that contains the IP:PORT of the AnalysisServer
            insertInitLibAnalysis(M); 

            // The following block is responsible for inserting two types of calls:
            // * update_vars: after each load or store instruction
            // * inst_notification: at the beginning of each basic block
            // The instrumentation is done by going through all the functions
            int f = 0;
            for (Module::iterator F = M.begin(), N = M.end(); F != N; ++F, ++f) {
                // We skip declarations because they don't have any line of code
                if (F->isDeclaration())
                    continue;

                int bb = 0;
                // For each basic block inside the current function (F)
                for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB, ++bb) {
                    int i = -1;
                    // This variable is set only if 'inst_notification' call was
                    // already inserted inside the current basic block.
                    int bb_notif = 0;
                    
                    bool isInitLibAnalysisBB = false;
                    
                    // For each instruction inside the current basic block (BB)
                    for (BasicBlock::iterator I = BB->begin(), J = BB->end(); I != J; ++I) {
                        // We are looking for the first basic block inside the main
                        // function to skip all the lines that were inserted before.
                        if (F->getName() == "main" && BB == F->begin()) {
                            if (I == BB->begin()) {
                                while (1) {
                                    ++I;
                                    if (!strcmp(I->getOpcodeName(), "call")) {
                                        CallInst *call = cast<CallInst>(I);
                                        Function *F = get_calledFunction(call);
                                        if (F && F->getName().str() == "init_libanalysis") {
                                            ++I; // This should pass over mpi_update_process_id.
                                            isInitLibAnalysisBB = true;
                                            break;
                                        }
                                    }
                                }
                                continue;
                            }
                        }

                        i++;
                        
                        // Currently we are ignoring all the 'landingpad' calls. 
                        // The 'landingpad' calls are used in C++ at the end of
                        // some basic blocks, but they are not a useful construction.
                        if (!strcmp(I->getOpcodeName(), "landingpad")) {
                            /* For the moment, ignore the entire BB. */
                            break;
                        }

                        // If the current instruction is a 'phi' instruction we should
                        // skip it because LLVM IR requires that all 'phi' instructions
                        // be placed at the beginning of the basic block and, therefore, 
                        // we cannot insert 'inst_notification' calls before.
                        if (!strcmp(I->getOpcodeName(), "phi")) {
                            // PHI instructions are always at the beginning of the BasicBlock 
                            continue;
                        }
                        

                        // If the current basic block does not have the 'inst_notification'
                        // call inserted, we insert it and set 'bb_notif' variable
                        if (!bb_notif) {
                            Instruction* instNotificationInstruction = insertInstNotification(M, BB, I, f, bb, i, "inst_notification");
                                            if (isInitLibAnalysisBB) {
                                                insertFAddr(M, BB, instNotificationInstruction);
                                        }
                            bb_notif = 1;
                        }
                    
                        // If the current basic block is a 'load' or a 'store' instruction
                        // we must insert 'update_vars' call.
                        if (!strcmp(I->getOpcodeName(), "load") ||
                                !strcmp(I->getOpcodeName(), "store")) {
                            // Insert after the current Instruction
                            insertLSValues(M, BB, I, f, bb, i);

                            // We must increment the instruction iterator in order to skip
                            // the just added instruction
                            I++; 
                        } else if (!strcmp(I->getOpcodeName(), "call")) {
                            // ret is 0 if the current instruction didn't need a hook
                            //     is 1 if an insertion after took place;
                            int ret = insertUpdateProcessID(M, BB, I, f, bb, i);

                            // here we inserted two instrumentation instructions for the MPI_Init call
                            if (ret) { I++; I++; }

                            // ret is 0 if the current instruction didn't need a hook
                            //     is 1 if an insertion after took place;
                            ret = insertMPIhooks(M, BB, I, f, bb, i);

                            // We must increment I if an instruction was added
                            if (ret)
                                I++;
                        }  
                            

                        // If the current instruction is a 'ret' instruction and the current
                        // function is the 'main' function, we should insert the 'end_app' call.
                        // This call is useful to notify libanalysis to dump extracted information
                        if (F->getName() == "main" && !strcmp(I->getOpcodeName(), "ret"))
                            insertEndApplication(M, BB, I);
                    }
                }
            }

            return false;
    }

};
}

char Analysis::ID = 0;
static RegisterPass<Analysis> X("analysis", "IBM Analysis Decoupled", false, false);
