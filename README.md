# IBM Platform-Independent Software Analysis

IBM Platform-Independent Software Analysis is a framework based on the LLVM compiler infrastructure that analyzes C/C++ and Fortran code at instruction, basic block and function level at application run-time. Its objective is to generate a software model for sequential and parallel (OpenMP and MPI) applications in a hardware-independent manner. Two examples of use cases for our framework are: 
  - Hardware-agnostic software characterization to support design decision to match hardware designs to applications.
  - Hardware design-space exploration studies by combining the software model with hardware performance models.

IBM Platform-Independent Software Analysis characterizes applications per thread and process and measures software properties such as: instruction-level parallelism, flow-control behavior, memory access pattern, and inter-process communication behavior. 
