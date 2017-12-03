include ../../Makefile.common

APPNAME="example"
COMPILATION_SCENARIO="default"

# In Makefile.common.commands, the files *.c are considered C and the files *.cc and *.cpp are considered C++
SOURCES_F=hello.f 

# All object files
SOURCES_BC=${SOURCES_F:.f=.bc}

# Source language to use for final code generation. It has to be C++ if any C++ source were used.
LANGUAGE=F

# Additional application specific flags for compilation and linking
APPADDCOMPILEFLAGS=
APPADDLDFLAGS=-lm -lgfortran

# Where to install the executables (*.nls will be analysis files)
INSTALL_FOLDER=../../profile/app1-fortran

include ../Makefile.common.commands


# command to test the compilation
test: exe-clang
	./exe-clang 


