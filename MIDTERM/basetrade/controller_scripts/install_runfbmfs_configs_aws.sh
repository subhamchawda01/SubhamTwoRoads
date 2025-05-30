#!/bin/bash

USAGE="$0 [temp_config]";

uid=`date +%N`;

temp_config="$HOME/modelling/fbmfswork/sample_config.txt";

if [ $# -gt 0 ] ; then temp_config=$1; fi;

ip=`cat ~/AWSScheduler/instance_cores.txt | awk '{if($2>0){ print $1; }}' | head -n1`; 
if [ `grep "^runfbmfs_queue " $HOME/AWSScheduler/AWSQueue.dat | wc -l` -le 0 ] ; then 
  ssh $ip "$HOME/basetrade_install/scripts/install_runfbmfs_configs.sh $temp_config $HOME/modelling/aws_gsq_store/runfbmfs_queue";
  cp $HOME/modelling/aws_gsq_store/runfbmfs_queue $HOME/AWSScheduler/queues/runfbmfs_queue;
  cp $HOME/modelling/aws_gsq_store/runfbmfs_queue /mnt/sdf/JOBS/queues/runfbmfs_queue;  #not needed for new scheduler
  $HOME/AWSScheduler/queues/queue_file_gen.sh $HOME/AWSScheduler/queues/runfbmfs_queue;
  echo "runfbmfs_queue 15 2 L" >> $HOME/AWSScheduler/AWSQueue.dat;
fi;


