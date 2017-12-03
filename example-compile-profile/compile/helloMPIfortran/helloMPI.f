
       



       program hello
       include 'mpif.h'

       
       integer numtasks, rank, len, ierr  
       character(MPI_MAX_PROCESSOR_NAME) hostname
     
       call MPI_INIT(ierr)
       if (ierr .ne. MPI_SUCCESS) then
          print *,'Error starting MPI program. Terminating.'
          call MPI_ABORT(MPI_COMM_WORLD, rc, ierr)
       end if
     
       call MPI_COMM_RANK(MPI_COMM_WORLD, rank, ierr)
       call MPI_COMM_SIZE(MPI_COMM_WORLD, numtasks, ierr)
       call MPI_GET_PROCESSOR_NAME(hostname, len, ierr)
       print *, 'Hello from process ',rank,' of ', numtasks,'.'
       print *, ' I am running on host: ',hostname, '.'
       
       print *, ' SQRT(rank): ',SQRT(rank+3.3)
 
       call MPI_FINALIZE(ierr)
       
       end
