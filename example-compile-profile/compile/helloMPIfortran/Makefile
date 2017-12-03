include ../../Makefile.common

APPNAME="example"
COMPILATION_SCENARIO="default"


# in Makefile.common.commands, the files *.c are considered C and the files *.cc and *.cpp are considered C++
SOURCES_F=helloMPI.f 

# All object files
SOURCES_BC=${SOURCES_F:.f=.bc}

# Source language to use for final code generation. It has to be C++ if any C++ source were used
LANGUAGE=C++

# Additional application specific flags for compilation and linking
APPADDCOMPILEFLAGS=
APPADDLDFLAGS=-lgfortran -L$(MPI_DIR) -lmpi -lmpi_mpifh

# where to install the executables (*.nls will be analysis files)
INSTALL_FOLDER=../../profile/helloMPIfortran

include ../Makefile.common.commands

# command to run to quickly test the compilation
test: exe-clang
	./exe-clang 


