#!/bin/bash

INST_FILE="/mnt/sdf/JOBS/all_instances.txt"
CORE_FILE="/mnt/sdf/JOBS/instance_cores.txt"

for a in ` grep running $INST_FILE | grep -v "10.0.0.11" | awk '{print $4}' ` ; do echo $a ; num_cores=`grep $a $CORE_FILE 2>/dev/null | awk '{print $2}'` ; num_running=`ls /mnt/sdf/JOBS/job_desc/COMMAND_$a* 2>/dev/null | wc -l ` ; num_done=`ls /mnt/sdf/JOBS/job_desc/DONE_$a* 2>/dev/null | wc -l `; echo $a" " $(($num_cores - $num_running + $num_done)) ; done | awk '{ for ( i = 0; i < $2; i = i + 1) print $1 }' > /mnt/sdf/JOBS/free_instances.txt
