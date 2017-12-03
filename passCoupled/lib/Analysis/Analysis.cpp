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
#define WAITALL_MPI_CALL 10
#define TESTALL_MPI_CALL 11
#define BCAST_MPI_CALL   12
#define IBCAST_MPI_CALL  13
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

// Used to map between analysis types and bits inside the 'flag' variable
// These defines must be _identical_ as in libanalysis.so
#define ANALYZE_ILP 1
#define ANALYZE_ILP_CTRL    2
#define ANALYZE_ILP_VERBOSE 4
#define ANALYZE_ILP_IGNORE_CTRL 8
#define ANALYZE_ITR 16
#define ANALYZE_DTR 32
#define ANALYZE_REG_COUNT   64
#define PRINT_LOAD_STORE    128
#define ANALYZE_MPI_CALLS   256
#define ANALYZE_ILP_TYPE    512
#define PRINT_DEBUG         1024
#define PRINT_BRANCH        2048
#define PRINT_BRANCH_COND       4096
#define ANALYZE_OPENMP_CALLS    8192
#define ANALYZE_MPI_MAP         16384
#define ANALYZE_MPI_DATA        32768
#define ANALYZE_MEM_FOOTPRINT   65536
#define ANALYZE_EXTERNALLIBS_CALLS  131072

using namespace llvm;

std::map<std::string, Value *> var_names;

// The following global variables are used for adding command line arguments
// to the instrumentation process. Each of them is defining the name of the
// argument, its type and a short description.
cl::opt<bool> ILPAnalyze("analyze-ilp", cl::desc("Enable ILP analysis"));
cl::opt<bool> ILPCtrl("ilp-ctrl", cl::desc("ILP with incremental control instructions"));
cl::opt<bool> ILPVerbose("ilp-verbose", cl::desc("ILP verbose output"));
cl::opt<bool> ILPIgnoreCtrl("ilp-ignore-ctrl", cl::desc("ILP without any control instruction"));

cl::opt<bool> DTRAnalyze("analyze-data-temporal-reuse", cl::desc("Enable data temporal reuse analysis"));
cl::opt<bool> DTRMemFootprint("analyze-memory-footprint", cl::desc("Enable memory footprint analysis - to be run in parallel with the DTR analysis. Default: disabled."),cl::init(0));
cl::opt<int> DTRCacheLineSize("data-cache-line-size", cl::desc("Set data cache line size. 0 is the equivalent of not using this option"), cl::init(0));
cl::opt<int> DTRResolution("data-reuse-distance-resolution", cl::desc("Set DTR resolution. 0 is the equivalent of not using this option"), cl::init(0));
cl::opt<int> DTREndResolution("data-reuse-distance-resolution-final-bin", cl::desc("Set final bin for DTR resolution. 0 is the equivalent of not using this option"), cl::init(0));

cl::opt<bool> ITRAnalyze("analyze-inst-temporal-reuse", cl::desc("Enable instruction temporal reuse analysis"));
cl::opt<int> ITRCacheLineSize("inst-cache-line-size", cl::desc("Set inst cache line size. 0 is equivalent of not using this option"), cl::init(0));
cl::opt<int> ITRInstSize("inst-size", cl::desc("Set inst line size. Mandatory if inst-cache-line-size is present"), cl::init(0));

cl::opt<bool> RCAnalyze("register-counting", cl::desc("Enable register counting analysis"));
cl::opt<bool> PrintLoadStore("print-load-store", cl::desc("Print load/store extended details"));

cl::opt<bool> MPICalls("mpi-stats", cl::desc("Enable MPI calls statistics"));
cl::opt<bool> MPIData("mpi-data", cl::desc("Enable measurement of data exchanged between the processes"));
cl::opt<bool> OpenMPCalls("openmp-stats", cl::desc("Enable OpenMP calls statistics"));

cl::opt<bool> ExternalLibraryCalls("external-library-call-count", cl::desc("Enable counting external library calls"));
cl::list<std::string> ExcludedFunctions("exclude", cl::desc("Exclude functions from analysis"), cl::CommaSeparated, cl::ZeroOrMore);
cl::list<std::string> IncludeFunctions("include", cl::desc("Include functions from analysis"), cl::CommaSeparated, cl::ZeroOrMore);

cl::opt<std::string> ApplicationName("app-name", cl::desc("Application code name"), cl::init("default-app"));
cl::opt<std::string> TestName("test-name", cl::desc("Testing scenario code name"), cl::init("default-values"));

cl::opt<std::string> PISAFileName("pisa-file-name", cl::desc("PISA output file name, if empty take name from environment (PISAFileName), if empty use stderr."), cl::init(""));
cl::opt<std::string> AddJSONData("add-json-data", cl::desc("Additional data to be added to the JSON output (AddJSONData), if empty, no additional data."), cl::init(""));

cl::opt<int> ILPType("ilp-type", cl::desc("ILP for type TYPE of instructions"), cl::init(0));
cl::opt<int> PrintDebug("msg-dbg", cl::desc("Print debug messages"), cl::init(0));
cl::opt<int> WindowSize("window-size", cl::desc("Set window size for ILP scheduler"), cl::init(0));
cl::opt<bool> BranchEntropy("branch-entropy", cl::desc("Enable dump towards computing branch entropy"), cl::init(false));
cl::opt<std::string> BranchEntropyFile("branch-entropy-file", cl::desc("Dump branch entropy trace to the specified file. Default is stdout."), cl::init(""));
cl::opt<bool> BranchEntropyCond("branch-entropy-cond", cl::desc("Enable dump towards computing branch entropy, only for conditional branches"), cl::init(false));
cl::opt<int> MaxExpectedNrOfThreads("max-expected-threads", cl::desc("Maximum expected number of threads"), cl::init(1));
cl::opt<std::string> MPISync("mpi-sync", cl::desc("Enable MPI sync for computing issue cycles"), cl::init(""));
cl::opt<std::string> MPIMap("mpi-map", cl::desc("Enable MPI mapping dump"), cl::init(""));

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

        static void sendSize(Module &M, Function::iterator BB, Instruction *I, std::string name, int value) {
            std::vector<Value *> args;
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), value));

            Constant *hook = M.getOrInsertFunction(name, 
                                                   Type::getVoidTy(M.getContext()),
                                                   Type::getInt32Ty(M.getContext()), 
                                                   (Type *)NULL);
            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");
            
#if LLVM_VERSION_MINOR > 7
            auto InsertPt = I->getIterator();
            BB->getInstList().insertAfter(InsertPt, new_inst);
#else
            BB->getInstList().insertAfter(I, new_inst);
#endif
        }

        static void excludeFunctions(Module &M, Function::iterator BB, Instruction *I) {
            IRBuilder<> builder(M.getContext());
            builder.SetInsertPoint((BasicBlock*)BB);

            Instruction *new_inst = I;
            for (unsigned i = 0; i < ExcludedFunctions.size(); i++) {
                std::vector<Value *> vec;
                Value * ActualPtrName = builder.CreateGlobalStringPtr(ExcludedFunctions[i].c_str());
                GetElementPtrInst * gepi = GetElementPtrInst::CreateInBounds(ActualPtrName, vec, "", (Instruction*)I);

                std::vector<Value *> args;
                args.push_back(gepi);

                // exclude_function(char *name)
                Constant *hook = M.getOrInsertFunction("exclude_function",
                        Type::getVoidTy(M.getContext()),
                        PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                        (Type *)NULL);

                new_inst = CallInst::Create(cast<Function>(hook), args, "");
                
#if LLVM_VERSION_MINOR > 7
                auto InsertPt = gepi->getIterator();
                BB->getInstList().insertAfter(InsertPt, new_inst);
#else
                BB->getInstList().insertAfter(gepi, new_inst);
#endif
            }

            if (strlen(MPISync.c_str()) > 0 || strlen(MPIMap.c_str()) > 0) {
                std::vector<Value *> v1;
                Value *anp = NULL;
                std::string hook_name = "mpi_sync_server_info";
                if (strlen(MPISync.c_str()) > 0) {
                    anp = builder.CreateGlobalStringPtr(MPISync.c_str());
                } else {
                    anp = builder.CreateGlobalStringPtr(MPIMap.c_str());
                    hook_name = "mpi_map_server_info";
                }
                GetElementPtrInst *anp_gepi = GetElementPtrInst::CreateInBounds(anp, v1, "", (Instruction*)I);

                std::vector<Value *> a1;
                a1.push_back(anp_gepi);
                Constant *h1 = M.getOrInsertFunction(hook_name,
                                                     Type::getVoidTy(M.getContext()),
                                                     PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                                                     (Type *)NULL);

                Instruction *ni1 = CallInst::Create(cast<Function>(h1), a1, "");
                
#if LLVM_VERSION_MINOR > 7
                auto InsertPt = anp_gepi->getIterator();
                BB->getInstList().insertAfter(InsertPt, ni1);
#else
                BB->getInstList().insertAfter(anp_gepi, ni1);    
#endif
            }

            /* Register application name */
            std::vector<Value *> v1;
            Value *anp = builder.CreateGlobalStringPtr(ApplicationName.c_str());
            GetElementPtrInst *anp_gepi = GetElementPtrInst::CreateInBounds(anp, v1, "", (Instruction*)I);

            std::vector<Value *> a1;
            a1.push_back(anp_gepi);
            Constant *h1 = M.getOrInsertFunction("app_name",
                    Type::getVoidTy(M.getContext()),
                    PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                    (Type *)NULL);

            Instruction *ni1 = CallInst::Create(cast<Function>(h1), a1, "");
            
#if LLVM_VERSION_MINOR > 7
            auto InsertPt = anp_gepi->getIterator();
            BB->getInstList().insertAfter(InsertPt, ni1);
#else
            BB->getInstList().insertAfter(anp_gepi, ni1);
#endif

            /* Register test name */
            std::vector<Value *> v2;
            Value *tnp = builder.CreateGlobalStringPtr(TestName.c_str());
            GetElementPtrInst *tnp_gepi = GetElementPtrInst::CreateInBounds(tnp, v2, "", (Instruction*)I);

            std::vector<Value *> a2;
            a2.push_back(tnp_gepi);
            Constant *h2 = M.getOrInsertFunction("test_name",
                                                 Type::getVoidTy(M.getContext()),
                                                 PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                                                 (Type *) NULL);

            Instruction *ni2 = CallInst::Create(cast<Function>(h2), a2, "");
            
#if LLVM_VERSION_MINOR > 7
            InsertPt = tnp_gepi->getIterator();
            BB->getInstList().insertAfter(InsertPt, ni2);
#else
            BB->getInstList().insertAfter(tnp_gepi, ni2);    
#endif

            /* Register PISA output name */
            std::vector<Value *> vPO;
            Value *pnp = builder.CreateGlobalStringPtr(PISAFileName.c_str());
            GetElementPtrInst *pnp_gepi = GetElementPtrInst::CreateInBounds(pnp, vPO, "", (Instruction*)I);

            std::vector<Value *> aPO;
            aPO.push_back(pnp_gepi);
            Constant *hPO = M.getOrInsertFunction("pisa_output",
                                                  Type::getVoidTy(M.getContext()),
                                                  PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                                                  (Type *) NULL);

            Instruction *niPO = CallInst::Create(cast<Function>(hPO), aPO, "");

#if LLVM_VERSION_MINOR > 7
            InsertPt = pnp_gepi->getIterator();
            BB->getInstList().insertAfter(InsertPt, niPO);
#else
            BB->getInstList().insertAfter(pnp_gepi, niPO);    
#endif

            /* Register AddJSONData file name */
            std::vector<Value *> vP1;
            Value *pnp1 = builder.CreateGlobalStringPtr(AddJSONData.c_str());
            GetElementPtrInst *pnp_gepi1 = GetElementPtrInst::CreateInBounds(pnp1, vP1, "", (Instruction*)I);

            std::vector<Value *> aP1;
            aP1.push_back(pnp_gepi1);
            Constant *hP1 = M.getOrInsertFunction("add_json_data",
                                                  Type::getVoidTy(M.getContext()),
                                                  PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                                                  (Type *) NULL);

            Instruction *niP1 = CallInst::Create(cast<Function>(hP1), aP1, "");

#if LLVM_VERSION_MINOR > 7
            InsertPt = pnp_gepi1->getIterator();
            BB->getInstList().insertAfter(InsertPt, niP1);
#else
            BB->getInstList().insertAfter(pnp_gepi1, niP1);    
#endif

            /* Done string parameter registration */

            for (unsigned i = 0; i < IncludeFunctions.size(); i++) {
                std::vector<Value *> vec;
                Value * ActualPtrName = builder.CreateGlobalStringPtr(IncludeFunctions[i].c_str());
                GetElementPtrInst * gepi = GetElementPtrInst::CreateInBounds(ActualPtrName, vec, "", (Instruction*)I);

                std::vector<Value *> args;
                args.push_back(gepi);

                // exclude_function(char *name)
                Constant *hook = M.getOrInsertFunction("include_function",
                                                        Type::getVoidTy(M.getContext()),
                                                        PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                                                        (Type *) NULL);

                new_inst = CallInst::Create(cast<Function>(hook), args, "");
                
#if LLVM_VERSION_MINOR > 7
                InsertPt = gepi->getIterator();
                BB->getInstList().insertAfter(InsertPt, new_inst);
#else
                BB->getInstList().insertAfter(gepi, new_inst);    
#endif

            }

                std::vector<Value *> v3;
                Value *befp = builder.CreateGlobalStringPtr(BranchEntropyFile.c_str());
                GetElementPtrInst *befp_gepi = GetElementPtrInst::CreateInBounds(befp, v3, "", (Instruction*)I);

                std::vector<Value *> a3;
                a3.push_back(befp_gepi);
                Constant *h3 = M.getOrInsertFunction("update_branch_entropy_file",
                                                     Type::getVoidTy(M.getContext()),
                                                     PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                                                     (Type *) NULL);

                Instruction *ni3 = CallInst::Create(cast<Function>(h3), a3, "");
                        
#if LLVM_VERSION_MINOR > 7
                InsertPt = befp_gepi->getIterator();
                BB->getInstList().insertAfter(InsertPt, ni3);
#else
                BB->getInstList().insertAfter(befp_gepi, ni3);    
#endif

            if (DTRCacheLineSize != 0)
                sendSize(M, BB, new_inst, "update_data_cache_line_size", DTRCacheLineSize);
            
            if (DTRResolution != 0)
                sendSize(M, BB, new_inst, "update_data_reuse_distance_resolution", DTRResolution);
            
            if (DTREndResolution != 0)
                sendSize(M, BB, new_inst, "update_data_reuse_distance_resolution_final_bin", DTREndResolution);
                
            if (ITRCacheLineSize != 0) {
                sendSize(M, BB, new_inst, "update_inst_cache_line_size", ITRCacheLineSize);
                if (ITRInstSize != 0)
                    sendSize(M, BB, new_inst, "update_inst_size", ITRInstSize);
            }

            if (ILPType >= 0)
                sendSize(M, BB, new_inst, "update_ilp_type", ILPType);

            if (PrintDebug == 1)
                sendSize(M, BB, new_inst, "enable_debug", PrintDebug);

            if (WindowSize)
                sendSize(M, BB, new_inst, "update_window_size", WindowSize);

            if (MaxExpectedNrOfThreads)
                sendSize(M, BB, new_inst, "update_max_expected_threads", MaxExpectedNrOfThreads);


        }


        static void insertFAddr(Module &M, Function::iterator BB, Instruction *I) {
            int f = 0;
            Instruction *new_inst = I;
            
            for (Module::iterator F = M.begin(), N = M.end(); F != N; ++F, ++f) {
                if (F->isDeclaration()){
                    continue;
                }

                std::vector<Type *> argsTy;

                // func_address(i32 x, ...) - (i32 x, real function address)
                FunctionType *ftype = FunctionType::get(Type::getVoidTy(M.getContext()), argsTy, true);
                Constant *hook = M.getOrInsertFunction("func_address", ftype);

                std::vector<Value *> args;

                // Insert IDs
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), f));
                args.push_back((Function*)F);

                Instruction *before = new_inst;
                new_inst = CallInst::Create(cast<Function>(hook), args, "");
                
#if LLVM_VERSION_MINOR > 7
                auto InsertPt = before->getIterator();
                BB->getInstList().insertAfter(InsertPt, new_inst);
#else
                BB->getInstList().insertAfter(before, new_inst);    
#endif

            }
            excludeFunctions(M, BB, new_inst);
        }

        static void insertGlobalModuleStr(Module &M, unsigned long long flags) {
            std::string str;
            raw_string_ostream rso(str);
            M.print(rso, NULL);

            Module::iterator F, N;
            for (F = M.begin(), N = M.end(); F != N; ++F)
                if (F->getName() == "main")
                    break;

            Function::iterator BB = F->begin();
            BasicBlock::iterator I = BB->begin();

            IRBuilder<> builder(M.getContext());
            builder.SetInsertPoint((BasicBlock*)BB);

            // insert global string with Module definition
            std::vector<Value *> vec;
            Value * ActualPtrName = builder.CreateGlobalStringPtr(str.c_str());
            GetElementPtrInst * gepi = GetElementPtrInst::CreateInBounds(ActualPtrName, vec, "", (Instruction*)I);

            std::vector<Value *> args;
            args.push_back(gepi);
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), flags));

            // init_libanalysis(char *, i32) -> (char *str_module, flags)
            Constant *hook = M.getOrInsertFunction("init_libanalysis",
                                                   Type::getVoidTy(M.getContext()),
                                                   PointerType::getUnqual(Type::getInt8Ty(M.getContext())),
                                                   Type::getInt32Ty(M.getContext()),
                                                   (Type *)NULL);

            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");

            // add any new instruction inside insertFAddr function
            insertFAddr(M, BB, gepi);
            BB->getInstList().insert(I, new_inst);
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
            Constant *hook = M.getOrInsertFunction("end_app", Type::getVoidTy(M.getContext()), (Type *)NULL);
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
            
#if LLVM_VERSION_MINOR > 7
            auto InsertPt = I->getIterator();
            BB->getInstList().insertAfter(InsertPt, new_inst);
#else
            BB->getInstList().insertAfter(I, new_inst);    
#endif

        }

        static int insertUpdateProcessID(Module &M, Function::iterator BB, BasicBlock::iterator I,
                int f, int bb, int i) {
            std::vector<Type *> argsTy;


            if (strcmp(I->getOpcodeName(), "call"))
                return 0;

            CallInst *call = cast<CallInst>(I);
            Function *_call = get_calledFunction(call);
            if (!_call || strncasecmp(_call->getName().str().c_str(), "MPI_", 4))
                return 0;

            FunctionType *ftype = FunctionType::get(Type::getVoidTy(M.getContext()), argsTy, true);
            Constant * hook = M.getOrInsertFunction("mpi_update_process_id", ftype);

            std::vector<Value *> args;

            const char *call_name = _call->getName().str().c_str();
            if (strcasecmp(call_name, "MPI_Init") && strcasecmp(call_name, "MPI_Init_"))
                return 0;

            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");
            
#if LLVM_VERSION_MINOR > 7
            auto InsertPt = I->getIterator();
            BB->getInstList().insertAfter(InsertPt, new_inst);
#else
            BB->getInstList().insertAfter(I, new_inst);    
#endif

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
            
#if LLVM_VERSION_MINOR > 7
            InsertPt = new_inst->getIterator();
            BB->getInstList().insertAfter(InsertPt, new_inst2);
#else
            BB->getInstList().insertAfter(new_inst, new_inst2);    
#endif

            return 1;
        }

        static int insertMPIhooks(Module &M, Function::iterator BB, BasicBlock::iterator &I,
                int f, int bb, int i) {
            std::vector<Type *> argsTy;


            if (strcmp(I->getOpcodeName(), "call"))
                return 0;

            CallInst *call = cast<CallInst>(I);
            Function *_call = get_calledFunction(call);
            /* Not only the call itself but also the called functions */
            if (!_call || strncasecmp(_call->getName().str().c_str(), "MPI_", 4))
                return 0;

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
            bool remove = false;

            
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
                // Indexes start at 0
                args.push_back(call->getArgOperand(3));
                // Insert the communication tag - 5th argument
                // Indexes start at 0
                args.push_back(call->getArgOperand(4));
                // Insert the MPI_comm - 6th argument
                // Indexes start at 0
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
                // Insert rank of the root process. - 4th argument
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
                // Insert rank of the root process - 6th argument
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 7th argument
                args.push_back(call->getArgOperand(6));
            } else if (!strcasecmp(call_name, "MPI_Ireduce") || !strcasecmp(call_name, "MPI_Ireduce_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), IREDUCE_MPI_CALL));
                // Insert count
                args.push_back(call->getArgOperand(2));
                // Insert the size of an element in the buffer (datatype)
                args.push_back(call->getArgOperand(3));
                // Insert rank of the root process - 6th argument
                args.push_back(call->getArgOperand(5));
                // Insrt the MPI_comm - 7th argument
                args.push_back(call->getArgOperand(6));
                // Insert the real memory address of the request - 8th argument
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
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), INIT_MPI_CALL));
                // Push argc argument
                args.push_back(call->getArgOperand(0));
            } else if (!strcasecmp(call_name, "MPI_Finalize") || !strcasecmp(call_name, "MPI_Finalize_")) {
                args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), FIN_MPI_CALL));
                remove = true; // Finalization is executed in libanalysisCoupled.cc.
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
                if(!remove) {

#if LLVM_VERSION_MINOR > 7
                    auto InsertPt = I->getIterator();
                    BB->getInstList().insertAfter(InsertPt, new_inst);
#else
                    BB->getInstList().insertAfter(I, new_inst);    
#endif
        
                }
                else {
                    BasicBlock::iterator Iold = I;
                    I--,
                    BB->getInstList().erase(Iold);

#if LLVM_VERSION_MINOR > 7
                    auto InsertPt = I->getIterator();
                    BB->getInstList().insertAfter(InsertPt, new_inst);
#else
                    BB->getInstList().insertAfter(I, new_inst);    
#endif                        
                }
                return 1;
            }
            return 0;
        }

        static void insertInstNotification(Module &M, Function::iterator BB, BasicBlock::iterator I,
                int f, int bb, int i, std::string type) {
            std::vector<Value *> args;
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), f));
            args.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), bb));

            Constant *hook = M.getOrInsertFunction(type, Type::getVoidTy(M.getContext()),
                                                   Type::getInt32Ty(M.getContext()), // Function ID
                                                   Type::getInt32Ty(M.getContext()), // Basic Block ID
                                                   (Type *) NULL);
            Instruction *new_inst = CallInst::Create(cast<Function>(hook), args, "");
            BB->getInstList().insert(I, new_inst);
        }


        // This is the function run by 'opt' tool for each Module.
        // It's the main function of the instrumentation process.
        virtual bool runOnModule(Module &M) {
            // 'flag' variable is used to select which type of analysis
            // we want to be active during the extraction of properties.
            // Each analysis type has reserved a bit from this flag.
            // In order to activate an analysis, one must set the
            // corresponding bit. The mapping between analysis types
            // and bits must be identical as in the library.
            unsigned long long flags = 0;

            if (ILPAnalyze)  {
                flags = ANALYZE_ILP;
                if (ILPCtrl)
                    flags |= ANALYZE_ILP_CTRL;
                if (ILPVerbose)
                    flags |= ANALYZE_ILP_VERBOSE;
                if (ILPIgnoreCtrl)
                    flags |= ANALYZE_ILP_IGNORE_CTRL;
                if (ILPType >= 0)
                    flags |= ANALYZE_ILP_TYPE;
            }
            if (PrintDebug)
                flags |= PRINT_DEBUG;
            if (DTRAnalyze) 
                flags |= ANALYZE_DTR;
            if (DTRMemFootprint) 
                flags |= ANALYZE_MEM_FOOTPRINT;	
            if (ITRAnalyze)
                flags |= ANALYZE_ITR;
            if (RCAnalyze)
                flags |= ANALYZE_REG_COUNT;
            if (PrintLoadStore)
                flags |= PRINT_LOAD_STORE;
            if (BranchEntropy)
                flags |= PRINT_BRANCH;
            if (BranchEntropyCond)
                flags |= PRINT_BRANCH_COND;
            if (MPICalls)
                flags |= ANALYZE_MPI_CALLS;
            if (MPIData)
                flags |= ANALYZE_MPI_DATA;
            if (OpenMPCalls)
                flags |= ANALYZE_OPENMP_CALLS;
            if (ExternalLibraryCalls)
                flags |= ANALYZE_EXTERNALLIBS_CALLS;
                

            IRBuilder<> builder(M.getContext());

            // This function inserts all the library calls required at the 
            // beginning of the 'main' function.
            // eg: init_libanalysis
            //     func_address
            //     update_inst_size
            //     update_inst_cache_line_size
            //     update_data_cache_line_size
            // It also creates the global string that contains the LLVM IR
            insertGlobalModuleStr(M, flags); 

            int f = 0;

            // The following block is responsible for inserting two types of calls:
            // * update_vars: after each load or store instruction
            // * inst_notification: at the beginning of each basic block
            // The instrumentation is done by going through all the functions
            for (Module::iterator F = M.begin(), N = M.end(); F != N; ++F, ++f) {
                // We skip declarations because they don't have any line of code
                if (F->isDeclaration())
                    continue;

                int bb = 0;
                // For each basic block inside the current function (F)
                for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB, ++bb) {
                    int i = -1;
                    std::vector<Instruction *> phi_instructions;

                    // This variable is set only if 'inst_notification' call was
                    // already inserted inside the current basic block.
                    int bb_notif = 0;

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
                                        if (F && F->getName().str() == "init_libanalysis")
                                            break;
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
                            phi_instructions.push_back((Instruction*)I);
                            continue;
                        }
                        

                        // If the current basic block does not have the 'inst_notification'
                        // call inserted, we insert it and set 'bb_notif' variable
                        if (!bb_notif) {
                            insertInstNotification(M, BB, I, f, bb, i, "inst_notification");
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
static RegisterPass<Analysis> X("analysis", "IBM Analysis Coupled Version", false, false);
