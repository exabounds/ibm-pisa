#!/bin/bash

$PISA_LIB_PATH/server -filename main.noInstrumentation.bc -ip 127.0.0.1 -portno 1100 -register-counting -mpi-stats -mpi-data -max-expected-threads=16 &> log &

sleep 3
mpirun -n 3 ./main.pisaDecoupled.nls 

sleep 10
../../prettyPrint.sh log log.json
