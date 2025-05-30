#!/bin/bash
#Script to adjust the number of cores allotted to a queue
if [ $# -lt 2 ]; then
        echo "USAGE: $0 <queuename> <num-cores>"
        exit 0
fi

scheduler_home=/home/dvctrader/AWSScheduler
queue_config=$scheduler_home/AWSQueue.dat
temp_queue_config=$scheduler_home/.AWSQueue.dat_temp
queue_name=$1
num_cores=$2

#Remove the corresponding queue
cat $queue_config | awk -vqueue=$queue_name '{if($1!=queue) print $0;}' > $temp_queue_config
#Add a new line corresponding to the queue (with updated number of cores)
cat $queue_config | awk -vqueue=$queue_name -vcores=$num_cores '{if($1==queue) { $2=cores; print $0;} }' >> $temp_queue_config

#Replace the original file with temporary
mv $temp_queue_config $queue_config
