#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char** argv) {
  MPI_Init(NULL, NULL);

  int procId;
  MPI_Comm_rank(MPI_COMM_WORLD, &procId);

  int data = 1;
  MPI_Bcast(&data, 1,MPI_INT, 0, MPI_COMM_WORLD);
  
  MPI_Finalize();
}
