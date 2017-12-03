#include <stdio.h>
#include <mpi.h>

int main (int argc, char* argv[]) {
  int rank, size,res;
  char myName[64];

  MPI_Init (&argc, &argv);                  
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);  
  MPI_Comm_size (MPI_COMM_WORLD, &size); 

  MPI_Get_processor_name(myName,&res);
  printf( "Hello from process %d of %d. I am running on host: %s.\n", rank, size, myName );
  
  MPI_Finalize();
  
  return 0;
}
