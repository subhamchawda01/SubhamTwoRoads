#!/bin/bash

AWS_DIR=$HOME/AWSScheduler;
instance_cores_file=$AWS_DIR/instance_cores.txt;
proc_status_file=$AWS_DIR/running_jobs;

#Check which servers are not reachable
for instance in `cat $instance_cores_file | awk '{print $1}'`
do
  procs=`awk -vins=$instance '{if($4==ins){print $_;}}' $proc_status_file | tr ' ' '~' | tr '\n' ' '`;
  ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=10 $instance "
  for proc in $procs; do
    proc=\`echo \$proc | tr '~' ' '\`; 
    instance=\`echo \$proc | cut -d' ' -f4\`;

    current_status=\`echo \$proc | cut -d' ' -f3\`;
    if [ \$current_status == FAILED ]; then 
      continue
    fi;
    
    dt=\`echo \$proc | awk '{print \$2}'\`;
    dt=\`expr \$dt / 1000000\`;
    dt=\`date -d @\$dt +%Y%m%d-%H:%M:%S\`;
    extra=\`echo \$proc | awk -vd=\$dt '{print \$1, d;}'\`;
    exec_with_args=\`echo \$proc | awk '{for (i=5;i<NF;i++) printf(\"%s \",\$i); print \$NF}'\`;
    running_instances=\`ps -eaf | grep -v 'grep' | grep -c \"\$exec_with_args\"\`;

    if [ \$running_instances -gt 1 ]; then
      echo \$extra MULTIPLE_RUNNING $instance \$exec_with_args 
    elif [ \$running_instances -gt 0 ]; then
      echo \$extra RUNNING $instance \$exec_with_args 
    fi; 
  done;
  ";
done 
