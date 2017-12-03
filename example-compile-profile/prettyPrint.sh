#!/bin/bash

if [ $# -ne 2 ]; then
    echo usage: $0 sourceFile destFile
    exit 1
fi

sed -i '/^thread_id,processor_id,current_index,type,subtype,completion_status,num_inst_diff,local_index_recv,other_id,other_index/d' $1 # Need better investigation.
sed -i '/^Server is ready.../d' $1
jq . $1 > $2
