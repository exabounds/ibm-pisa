#!/bin/bash

# set to $LLVM_ROOT/llvm-install
export PISA_ROOT=/home/$USER
export LLVM_INSTALL=$PISA_ROOT/llvm-install
export LLVM_BUILD=$PISA_ROOT/llvm-build
export LLVM_SRC=$PISA_ROOT/llvm-3.4

# set to $OPENMP_DIR
export OPENMP_DIR=$PISA_ROOT/libomp_oss

# set to $MPI_DIR
export MPI_DIR=$PISA_ROOT/openmpi-1.8.6-install

# set to include dir of rapidjson library
RAPID_JSON=$PISA_ROOT/rapidjson/include

export PATH=$MPI_DIR/bin:$LLVM_INSTALL/bin:$PATH

export C_INCLUDE_PATH=$MPI_DIR/include:$LLVM_INSTALL/include:$OPENMP_DIR/exports/common/include
export C_INCLUDE_PATH=$RAPID_JSON:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=$MPI_DIR/include:$LLVM_INSTALL/include:$OPENMP_DIR/exports/common/include
export CPLUS_INCLUDE_PATH=$RAPID_JSON:$CPLUS_INCLUDE_PATH

export LIBRARY_PATH=$MPI_DIR/lib:$MPI_DIR/lib/openmpi:$LLVM_INSTALL/lib:$OPENMP_DIR/exports/lin_32e/lib
export LD_LIBRARY_PATH=$MPI_DIR/lib:$MPI_DIR/lib/openmpi:$LLVM_INSTALL/lib:$OPENMP_DIR/exports/lin_32e/lib
export LD_RUN_PATH=$MPI_DIR/lib:$MPI_DIR/lib/openmpi:$LD_RUN_PATH

# AFTER building the analysis pass:

# Set ANALYSIS_LIB_PATH
export COUPLED_PASS_PATH=$PISA_ROOT/analysis-install/lib
# export DECOUPLED_PASS_PATH=$PISA_ROOT/PISApass-decoupled-install/lib

# Set LIB_PATH
export PISA_LIB_PATH=$PISA_ROOT/ibm-pisa/library

export LD_LIBRARY_PATH=$PISA_LIB_PATH:$LD_LIBRARY_PATH

## This variable is used for automatic generation and verification of tests outputs
export PISA_EXAMPLES=$PISA_ROOT/ibm-pisa/example-compile-profile

## Analyze FORTRAN code with Dragonegg + LLVM-3.5.2
# export GCC=gcc-4.8
# export CC=gcc-4.8
# export CXX=g++-4.8
# export CFORTRAN=gcc-4.8
# export DRAGONEGG_PATH=$PISA_ROOT/dragonegg-3.5.2.src
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PISA_LIB_PATH:$DRAGONEGG_PATH
# export LIBRARY_PATH=$LIBRARY_PATH:$PISA_LIB_PATH:$DRAGONEGG_PATH

## Additional PISA inputs and outputs
# export PISAFileName=output.json
# export AddJSONData=header.json

## Print PISA output in JSON pretty print
export PRETTYPRINT=$PISA_ROOT/ibm-pisa/example-compile-profile/prettyPrint.sh
