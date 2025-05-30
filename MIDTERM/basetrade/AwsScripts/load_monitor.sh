#!/bin/bash
#Monitors load across all workers, and adjusts number of cores accordingly
cd /home/dvctrader/AWSScheduler
core_file=/home/dvctrader/AWSScheduler/instance_cores.txt
temp_core_file=/home/dvctrader/AWSScheduler/.instance_cores.tmp
err_log_file=/home/dvctrader/AWSScheduler/load_monitor_log.txt

if [ -e /home/dvctrader/AWSScheduler/lockfile ]; then
	#Scheduler is running now => don't update
	exit 1;
fi

if [ -e $temp_core_file ]; then
	#Another instance is probably running
	exit 1;
fi
#Must not leave without deleting temporary file
trap "rm -rf $temp_core_file" EXIT
touch $temp_core_file

echo "--- Adjusting cores ---" >> $err_log_file

while read line
do
	str=$line
	echo "Read $str" >> $err_log_file
	worker=`echo $str | awk '{print $1}'`
	cores=`echo $str | awk '{print $2}'`
	echo "Initial allocation: $worker $cores" >> $err_log_file

	#Get 5 min load avg at this worker
	max_worker_load=`ssh -o ConnectTimeout=60 -n $worker uptime | tr , ' ' | head -1 | awk '{max_load=$11; printf "%d\n",max_load;}'`

	#Stop scheduling if load>50% else schedule processes 
	if [ "$max_worker_load" -gt 50 ]; then cores=0; else cores=25; fi;

	echo "$worker $cores" >> $temp_core_file
	echo "New allocation: $max_worker_load $worker $cores" >> $err_log_file
done < "$core_file"

cat $temp_core_file >> $err_log_file
#Core adjustment done successfully=>replace original file
mv $temp_core_file $core_file
