include ../../Makefile.common 

APPNAME="example"
COMPILATION_SCENARIO="default"


# in Makefile.common.commands, the files *.c are considered C and the files *.cc and *.cpp are considered C++
SOURCES_C=bcastProgram.c
SOURCES_CC=
SOURCES_CPP=

# All object files
SOURCES_BC=${SOURCES_C:.c=.bc}
SOURCES_BC+=${SOURCES_CC:.cc=.bc}
SOURCES_BC+=${SOURCES_CPP:.cpp=.bc}

# Source language to use for final code generation. It has to be C++ if any C++ source were used
LANGUAGE=C

APPADDCOMPILEFLAGS=
APPADDLDFLAGS=-lmpi 

# where to install the executables (*.nls will be analysis files)
INSTALL_FOLDER=../../profile/helloMPI.c

include ../Makefile.common.commands

# command to test the compilation
test: exe-clang
	./exe-clang 




