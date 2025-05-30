#!/bin/bash

FIND_INST_SCRIPT="/home/dvctrader/controller_scripts/find_instances.sh"
INST_FILE="/mnt/sdf/JOBS/all_instances.txt"
INST_FILE_BK=$INST_FILE".bk"

CORE_FILE="/mnt/sdf/JOBS/instance_cores.txt"
CORE_FILE_BK=$CORE_FILE".bk"

$FIND_INST_SCRIPT > $INST_FILE_BK
md5_1=`md5sum $INST_FILE | awk '{print $1}'`
md5_2=`md5sum $INST_FILE_BK | awk '{print $1}'`

if [ "$md5_1" != "$md5_2" ] ; then
   for ip in `cat $INST_FILE_BK |grep running |  awk '{print $4}'`; do echo -n $ip" "; ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $ip nproc 2>/dev/null ; done > $CORE_FILE_BK
   mv $CORE_FILE_BK $CORE_FILE
   mv $INST_FILE_BK $INST_FILE
fi


grep running $INST_FILE | grep -v "10.0.0.11" | awk '{print $4}' | while read a ; do num_cores=`grep $a $CORE_FILE 2>/dev/null | awk '{print $2}'` ; num_running=`ls /mnt/sdf/JOBS/job_desc/COMMAND_$a* 2>/dev/null | wc -l ` ; num_done=`ls /mnt/sdf/JOBS/job_desc/DONE_$a* 2>/dev/null | wc -l `; echo $a" " $(($num_cores - $num_running + $num_done)) ; done | awk '{ for ( i = 0; i < $2; i = i + 1) print $1 }' > /mnt/sdf/JOBS/free_instances.txt

perl /home/dvctrader/controller_scripts/run_jobq.pl
