#!/bin/bash
FILE_DIR=$HOME/AWSScheduler
instance_cores_file=$FILE_DIR/instance_cores.txt
tmp_instance_cores=$FILE_DIR/tmp_instance_cores.txt
proc_status_file=$FILE_DIR/running_jobs
tmp_proc_status=$FILE_DIR/tmp_proc_status
debug_file=$FILE_DIR/proc_debug_file
removed_queues_file=$FILE_DIR/AWSDoneQueue.dat
queues_file=$FILE_DIR/AWSQueue.dat
tmp_queues_file=$FILE_DIR/tmp_AWSDoneQueue.dat
done_jobs=$FILE_DIR/done_jobs
core_adjusted_file=$FILE_DIR/instance_cores.txt_adjusted
all_instances_file=/mnt/sdf/JOBS/all_instances.txt

#echo "--------" >> $debug_file
rightnow=`date`
echo "------- $rightnow --------" >> $done_jobs
#cat $proc_status_file >> $debug_file
truncate -s 0 $tmp_proc_status #Reset new process status file

#Check which servers are not reachable
for instance in `cat $instance_cores_file | awk '{print $1}'`
do
  #cmd="ssh  -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o ConnectTimeout=10 $instance ls>/dev/null"
  cmd="ssh  -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o ConnectTimeout=240 $instance ps -efH"
  $cmd > $FILE_DIR/processes_$instance 2>/dev/null  
  exit_status=$?
  echo "ssh status: $exit_status for $instance"
  if [ $exit_status -eq 255 ]; then #simple ssh failed
    #send a mail here
    echo "AWS Scheduler: Machine $instance not reachable. Removing from worker list"  >> removing.txt
    #/bin/mail -s "Removing AWS Worker as it is not reachable: $instance" -r chandan.kumar@tworoads.co.in chandan.kumar@tworoads.co.in hardik@circulumvite.com < /dev/null
    /bin/mail -s "Removing AWS Worker as it is not reachable: $instance" -r nseall@tworoads.co.in nseall@tworoads.co.in < /dev/null
    grep -v "$instance" $instance_cores_file > $tmp_instance_cores #removing the unreachable server
    cp $tmp_instance_cores $instance_cores_file
    rm $tmp_instance_cores
    grep -v "$instance" $all_instances_file > $tmp_instance_cores #removing the unreachable server
    cp $tmp_instance_cores $all_instances_file
    rm $tmp_instance_cores
  fi
done 

current_time=`date +%s`
for line in `cat $proc_status_file | tr ' ' '~'`
do
  proc=`echo $line | tr '~' ' '`
  extra=`echo $proc | awk '{for (i=1;i<3;i++) printf("%s ",$i)}'`
  current_status=`echo $proc | cut -d' ' -f3`
  instance=`echo $proc | cut -d' ' -f4`
  exec_with_args=`echo $proc | awk '{for (i=5;i<NF;i++) printf("%s ",$i); print $NF}'`
  
  if [ $current_status == "FAILED" ]; then #no need to consider failed processes
    continue
  fi
  
  instance_reachable=`grep -c "$instance " $instance_cores_file`
  if [ $instance_reachable -lt 1 ]; then
    echo "$extra FAILED $instance $exec_with_args"  >> $tmp_proc_status #Instance was not reachable => job probably failed
    continue
  fi
  proc_file_=$FILE_DIR/processes_$instance
  #cmd="ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=10 $instance ps -efH | grep -v 'grep' | grep -c '$exec_with_args'"
  cmd=`grep "$exec_with_args\$" $proc_file_ | wc -l`
  running_instances=$cmd
  if [ $running_instances -gt 1 ]; then
      echo "$extra MULTIPLE_RUNNING $instance $exec_with_args" >> $tmp_proc_status
    elif [ $running_instances -gt 0 ]; then
      echo "$extra RUNNING $instance $exec_with_args" >> $tmp_proc_status
    else
      echo "$extra $current_time COMPLETE $instance $exec_with_args" >> $done_jobs
  fi  #If no trace of process, and instance is reachable, then we assume that the job has completed and we discard the process metadata line
done
cp $tmp_proc_status $proc_status_file
rm $tmp_proc_status  

#Update number of cores based on present load and number of procs running
rm -rf $core_adjusted_file
for instance in `cat $instance_cores_file | awk '{print $1}'`
do
  #Get 5 min load avg at this worker
  worker_load=`ssh -o ConnectTimeout=60 -n $instance uptime | tr , ' ' | head -1 | awk '{max_load=$11; printf "%d\n",max_load;}'`
  num_procs_running=`grep -c "RUNNING $instance " $proc_status_file`
  #Adjust num of available cores on the basis of these
  #available_cores=min ( 50 - 5min_load + curr_procs_scheduled , 25 )
  load_based_cores="$((50+$num_procs_running-$worker_load))"
  available_cores=30
  if [ "$load_based_cores" -lt "$available_cores" ]; then
    available_cores=$load_based_cores
  fi
  echo "$instance $available_cores" >> $core_adjusted_file
done

old_machine_count=`wc -l $instance_cores_file | awk '{print $1}' | tail -1`
new_machine_count=`wc -l $core_adjusted_file | awk '{print $1}' | tail -1`
if [ "$new_machine_count" -eq "$old_machine_count" ]; then
  echo "The generated instance file is fine $new_machine_count $old_machine_count. Replacing!"
  mv $core_adjusted_file $instance_cores_file
else
  echo "The generated instance file is corrupt $new_machine_count $old_machine_count. Ignoring!"
fi

#Remove queues which are done
for line in `cat $removed_queues_file | awk '{print $2 }'`
do
  grep -v "^$line " $queues_file > $tmp_queues_file
  cp $tmp_queues_file $queues_file
  rm $tmp_queues_file
done
rm -rf $removed_queues_file
