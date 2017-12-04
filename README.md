# IBM Platform-Independent Software Analysis

IBM Platform-Independent Software Analysis is a framework based on the LLVM compiler infrastructure that analyzes C/C++ and Fortran code at instruction, basic block and function level at application run-time. Its objective is to generate a software model for sequential and parallel (OpenMP and MPI) applications in a hardware-independent manner. Two examples of use cases for this framework are: 

  - Hardware-agnostic software characterization to support design decision to match hardware designs to applications.
  - Hardware design-space exploration studies by combining the software model with hardware performance models.

IBM Platform-Independent Software Analysis characterizes applications per thread and process and measures software properties such as: instruction-level parallelism, flow-control behavior, memory access pattern, and inter-process communication behavior. 

A detailed description of the IBM Platform-Independent Software Analysis tool can be found in the following paper:
[IBM-PISA IJPP Paper](https://doi.org/10.1007/s10766-016-0410-0).
```
@article{DBLP:journals/ijpp/AnghelVMJD16,
  author    = {Andreea Anghel and
               Laura Mihaela Vasilescu and
               Giovanni Mariani and
               Rik Jongerius and
               Gero Dittmann},
  title     = {An Instrumentation Approach for Hardware-Agnostic Software Characterization},
  journal   = {International Journal of Parallel Programming},
  volume    = {44},
  number    = {5},
  pages     = {924--948},
  year      = {2016},
  url       = {https://doi.org/10.1007/s10766-016-0410-0},
  doi       = {10.1007/s10766-016-0410-0},
  timestamp = {Wed, 26 Jul 2017 16:39:50 +0200},
  biburl    = {http://dblp.org/rec/bib/journals/ijpp/AnghelVMJD16},
  bibsource = {dblp computer science bibliography, http://dblp.org}
}
```

More related publications are listed on the project website:
[IBM-PISA-related projects](http://researcher.watson.ibm.com/researcher/view_group_pubs.php?grp=6395).


## Getting Started

Four LLVM versions are currently supported: 3.4 , 3.5.2 , 3.7 and 3.8. IBM Platform-Independent Software Analysis has been mostly tested with LLVM 3.4. The user might encounter issues with the other versions when for instance instrumenting source code compiled with the '-g' flag. LLVM 3.5.2 supports also FORTRAN code by using DRAGONEGG which has been tested with gcc-4.7.4. If the user needs to analyze FORTRAN code, use LLVM 3.5.2 with DRAGONEGG. LLVM 3.7 supports PowerPC backend.

The next section describes how to install the IBM Platform-Independent Software Analysis tool with LLVM 3.4 for Ubuntu OS. Note that the LLVM version is automatically identified at compilation time. Make sure that $LLVM_INSTALL/bin/clang is set to the right LLVM version.

## Installation Instructions 

```sh
$ LLVM_ROOT=$(pwd)

# Get the LLVM source code
$ wget http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz
$ tar xzvf llvm-3.4.src.tar.gz

# Configure, compile and install LLVM
$ export LLVM_ENABLE_THREADS=1
$ cd $LLVM_ROOT
$ mkdir llvm-build
$ mkdir llvm-install
$ cd llvm-build
$ ../llvm-3.4/configure --enable-optimized --prefix=$LLVM_ROOT/llvm-install
# for LLVM 3.5 or newer also add --enable-cxx11=yes
$ make -j4    
$ make install

# Configure, compile and install clang
# We will use clang with openMP support (x86_64 architectures). 
# For other architectures use the official clang version.
$ cd $LLVM_ROOT
$ git clone https://github.com/clang-omp/clang llvm-3.4/tools/clang
$ cd llvm-3.4/tools/clang
$ git checkout 34 # clang version for LLVM 3.4
# Rebuild LLVM (configure, compile and install)

# OpenMP installation
# Download the OpenMP runtime library and extract the archive:
# https://www.openmprtl.org/download#stable-releases
$ cd $LLVM_ROOT
$ wget https://www.openmprtl.org/sites/default/files/libomp_20160808_oss.tgz
$ tar -xzvf libomp_20160808_oss.tgz
$ cd libomp_oss
$ OPENMP_DIR=$(pwd)
$ make compiler=gcc

# OpenMPI installation
# Install OpenMPI and extract the archive (currently tested versions: 1.8.6 and 1.10.2):
# http://www.open-mpi.org/software/ompi/
$ cd $LLVM_ROOT
$ wget https://www.open-mpi.org/software/ompi/v1.10/downloads/openmpi-1.10.2.tar.gz
$ tar -xzvf openmpi-1.10.2.tar.gz
# Go to the openmpi-X.X.X folder.
$ cd openmpi-1.10.2
$ MPI_DIR=$(pwd)
$ ./configure --prefix $MPI_DIR
$ make all install

# RapidJSON installation 
# If cmake is not install, run: sudo apt-get install cmake
$ git clone https://github.com/miloyip/rapidjson
$ cd rapidjson
$ mkdir build
$ cd build    
$ cmake ..
$ make
# The user might need to edit the file 'rapidjson/build/example/CMakeFiles/lookaheadparser.dir/flags.make':
# Remove flags -Weffc++ and -Wswitch-default

# Configure and compile the analysis pass and the library
$ cd $LLVM_ROOT
$ git clone PATH_TO_PISA_GIT
$ cd ibm-pisa
$ ANALYSIS_ROOT_DIR=$(pwd)
# Copy my_env.sh and set the local paths.
# (Re)Edit the my_env.sh file to change the paths accordingly.
$ source my_env.sh

# Install the LLVM Pass (this pass instruments the LLVM bitcode with library calls)
# First, prepare the passCoupled folder (the same applies for passDecoupled) to have the following structure:
$ export TMP_LLVM_SAMPLE_SRC=$LLVM_ROOT/llvm-3.4/projects/sample
$ cd passCoupled

    Makefile                    copy $TMP_LLVM_SAMPLE_SRC/Makefile and change DIRS from 'lib tools' to 'lib'
    Makefile.common.in          copy $TMP_LLVM_SAMPLE_SRC/Makefile.common.in, change PROJECT_NAME to 'Analysis' and add PROJ_VERSION=0.1
    Makefile.llvm.config.in     copy $TMP_LLVM_SAMPLE_SRC/Makefile.llvm.config.in 
    Makefile.llvm.rules         copy $TMP_LLVM_SAMPLE_SRC/Makefile.llvm.rules

    lib/                        already created 
        Makefile                copy $TMP_LLVM_SAMPLE_SRC/lib/Makefile and change DIRS to 'Analysis'
        Analysis/               already created
            Analysis.cpp        already provided
            Makefile            copy $TMP_LLVM_SAMPLE_SRC/lib/sample/Makefile, change LIBRARYNAME to 'Analysis' and add LOADABLE_MODULE=1
            
    autoconf/                   copy $TMP_LLVM_SAMPLE_SRC/autoconf
        AutoRegen.sh            no changes required
        config.guess            no changes required
        config.sub              no changes required
                    
        configure.ac            change AC_INIT by replacing 'SAMPLE' with 'ANALYSIS' and by adding version number (0.01) and email address
                                change LLVM_SRC_ROOT to point to $LLVM_SRC ($LLVM_ROOT/llvm-3.4)
                                change LLVM_OBJ_ROOT to point to $LLVM_BUILD ($LLVM_ROOT/llvm-build)
                                change AC_CONFIG_MAKEFILE(lib/sample/Makefile) by replacing 'sample' with 'Analysis'
                                change AC_CONFIG_MAKEFILE(tools/Analysis/Makefile) by replacing 'sample' with 'Analysis'
                                            
        ExportMap.map           no changes required
        install-sh              no changes required
        LICENSE.txt             no changes required
        ltmain.sh               no changes required
                    
        m4/                     already copied with the autoconf/ folder 
        mkinstalldirs           no changes required    
        
# For the first installation of the pass, copy the full passCoupled (or passDecoupled) folder.
$ cp -r passCoupled $LLVM_ROOT/llvm-3.4/projects/Analysis
        
# After each Analysis.cpp update, just copy the Analysis.cpp file from passCoupled (or passDecoupled).
# Otherwise go to the next instruction.             

$ cd $LLVM_ROOT/llvm-3.4/projects/Analysis/autoconf        
# modify LLVM_SRC_ROOT and LLVM_OBJ_ROOT in /projects/Analysis/autoconf/configure.ac:
# LLVM_SRC_ROOT to $LLVM_ROOT/llvm-3.4 (or $LLVM_SRC is previously set in my_env.sh)
# LLVM_OBJ_ROOT to $LLVM_ROOT/llvm-build (or $LLVM_BUILD is previously set in my_env.sh)
# The user might need to run 'chmod u+x' on AutoRegen.sh.
# If autoconf is not installed, the user should run 'sudo apt-get install autoconf'
$ ./AutoRegen.sh
        
$ cd $LLVM_ROOT
$ mkdir analysis-build
$ mkdir analysis-install
        
$ cd analysis-build
$ $LLVM_ROOT/llvm-3.4/projects/Analysis/configure --with-llvmsrc=$LLVM_ROOT/llvm-3.4 --with-llvmobj=$LLVM_ROOT/llvm-build --prefix=$LLVM_ROOT/analysis-install
# for LLVM 3.5 or newer also add --enable-cxx11=yes
        
# Run 'chmod u+x $LLVM_ROOT/llvm-3.4/projects/Analysis/autoconf/mkinstalldirs' for the following command to run succesfully. 
$ make && make install
        
# Library
$ cd $ANALYSIS_ROOT_DIR/library
# If the boost libraries are not installed, run 'sudo apt-get install libboost-all-dev'.
$ make coupled
        
# In the 'my_env.sh' file, set COUPLED_PASS_PATH, PISA_LIB_PATH, PISA_EXAMPLES and PRETTYPRINT.
# export COUPLED_PASS_PATH=$PISA_ROOT/analysis-install/lib
# export PISA_LIB_PATH=$PISA_ROOT/ibm-pisa/library
# export LD_LIBRARY_PATH=$PISA_LIB_PATH:$LD_LIBRARY_PATH
# export PISA_EXAMPLES=$PISA_ROOT/ibm-pisa/example-compile-profile
# export PRETTYPRINT=$PISA_ROOT/ibm-pisa/example-compile-profile/prettyPrint.sh
# After setting these variables, run 'source my_env.sh'.  
```

## Testing the Installation
```sh
$ cd $ANALYSIS_ROOT_DIR/example-compile-profile/compile/app0
$ make pisa (to generate the instrumented binary)
# If -lcurses is not found, run 'sudo apt-get install libncurses5-dev'.
$ export PISAFileName=out
$ export OMP_NUM_THREADS=1
$ ./main.pisaCoupled.nls
$ vi out (to check the characterization results)
```
