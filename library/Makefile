.PHONY: clean coupled decoupled

CLANG=$(LLVM_INSTALL)/bin/clang
# Only 3.X where X is 4, 5.2, or 7
LLVM_VERSION=$(shell $(CLANG) --version | grep version | sed "s/.*version \([0-9]*\.[0-9]*\).*/\1/")
LLVM_VERSION_MAJOR=$(word 1, $(subst ., ,$(LLVM_VERSION)))
LLVM_VERSION_MINOR=$(word 2, $(subst ., ,$(LLVM_VERSION)))
CXX?=g++

CPPFLAGS=-I$(LLVM_INSTALL)/include -I$(LLVM_INSTALL)/include/llvm/Support -I$(MPI_DIR)/include -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -DLLVM_VERSION_MINOR=$(LLVM_VERSION_MINOR) -g
CXXFLAGS=-W -Wall -O3 -DLLVM_VERSION_MINOR=$(LLVM_VERSION_MINOR) -std=c++11 -g

LLVM_LIBS=$(shell llvm-config --libs)
LLVM_LDFLAGS=$(shell llvm-config --ldflags)

#ifneq ($(LLVM_VERSION_MINOR),4)
#   CPPFLAGS+= -std=c++11
#endif

# All sources indifferently on the fact that they are from coupled or decoupled version
SRCS=libanalysisCoupled.cc WKLDchar.cc InstructionAnalysis.cc ILP.cc InstructionMix.cc DataTempReuse.cc InstTempReuse.cc RegisterCount.cc LoadStoreVerbose.cc MPIstats.cc MPIdata.cc MPImap.cc BranchEntropy.cc splay.cc OpenMPstats.cc utils.cc server.cc safe_queue.cc JSONdumping.cc JSONmanager.cc MPIcnfSupport.cc ExternalLibraryCount.cc

OBJS=$(subst .cc,.o,$(SRCS))

# Only the objects of this specific software (coupled) version
COUPLEDOBJ=libanalysisCoupled.o WKLDchar.o InstructionAnalysis.o ILP.o InstructionMix.o DataTempReuse.o InstTempReuse.o RegisterCount.o LoadStoreVerbose.o MPIstats.o MPIdata.o MPImap.o BranchEntropy.o splay.o OpenMPstats.o utils.o safe_queue.o JSONmanager.o MPIcnfSupport.o ExternalLibraryCount.o

# Only the objects of this specific software (decoupled) version
DECOUPLEDOBJ=libanalysisDecoupled.o utils.o MPIcnfSupport.o

#server.o: server.cc
#   $(CXX) -c server.cc  -I /home/user/libboost/boost_1_53_0/

SERVEROBJ=server.o WKLDchar.o InstructionAnalysis.o ILP.o InstructionMix.o DataTempReuse.o InstTempReuse.o RegisterCount.o LoadStoreVerbose.o MPIstats.o MPIdata.o MPImap.o BranchEntropy.o splay.o utils.o OpenMPstats.o safe_queue.o JSONmanager.o ExternalLibraryCount.o

all: coupled decoupled

decoupled: libanalysisDecoupled.so server

coupled: libanalysisCoupled.so

libanalysisCoupled.so: $(COUPLEDOBJ) 
	$(CXX) -shared -o $@ $(COUPLEDOBJ)

libanalysisDecoupled.so: $(DECOUPLEDOBJ) 
	$(CXX) -shared -o $@ $(DECOUPLEDOBJ)

server: $(SERVEROBJ)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $(SERVEROBJ) $(LLVM_LDFLAGS)  $(LLVM_LIBS) -lm -lpthread -ldl -lcurses -lstdc++ -lc  -lmpi -lmpi_cxx -liomp5

mpi_sync_server: mpi_sync_server.c
	gcc $(CXXFLAGS) -o $@ $<

%.o: %.cc
	$(CXX) -fPIC -c -fopenmp -o $@ $< $(CXXFLAGS) $(CPPFLAGS)

clean:
	rm -f *.o *.so server
