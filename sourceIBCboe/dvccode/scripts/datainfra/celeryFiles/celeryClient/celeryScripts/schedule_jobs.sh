#!/bin/bash
BASE_PATH='/media/shared/ephemeral16/celeryCmnds/'
test "$(ls -A "$BASE_PATH" 2>/dev/null)" || exit 0;

OVERALL_MAX=2000
SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"

while true
do

  CURR_JOBS=`ssh $SSH_VARS ec2-user@52.91.139.132 "sudo /usr/sbin/rabbitmqctl list_queues -p vhostClient name messages | grep -w autoscalegroupmanual" | awk -F' ' '{print $NF}'`

  if [ $CURR_JOBS -ge $OVERALL_MAX ]; then
    exit 0;
  fi
  MAX_SCHEDULE=$(expr $OVERALL_MAX - $CURR_JOBS)
  #contains all files which are considered in this run, and still have more jobs
  tmp_log="/media/shared/ephemeral16/scheduler_log_tmp.txt"
  log_file="/media/shared/ephemeral16/scheduler_log.txt"
  init_list="/media/shared/ephemeral16/scheduler_init.txt"
  curr=0
  num=0
  > $tmp_log

  if [ ! -f $log_file ]; then
	ls $BASE_PATH > $log_file
  fi
  contents=`cat $log_file`
  if [ "$contents" == "" ]; then
	ls $BASE_PATH > $log_file
  fi

  cd $BASE_PATH
  for file in `cat $log_file`; do
	if [ $curr -lt $MAX_SCHEDULE ]; then
		ans=20
		total=`wc -l  < $BASE_PATH/$file`
		if [ $total -lt $ans ]; then
			ans=$total
		fi
 		head -n$ans $file > /spare/local/logs/celery_tmp_file
	    if [ $ans -eq $total ]; then
    	    rm $file;
	    else
    	    tail -n +$(expr $ans + 1) $file > ${file}.tmp
	        mv ${file}.tmp $file
		    echo $file >> $tmp_log
	    fi

    	echo "`date`, File: $file, Num Cmnds: $ans" >> /spare/local/logs/celery_scheduler
	    /home/dvctrader/celeryFiles/celeryClient/celeryScripts/run_my_job.py -f /spare/local/logs/celery_tmp_file -m 1 -n dvctrader -s 1 -i 1 >>  /spare/local/logs/celery_scheduler
		curr=$(($curr + $ans))
		num=$(($num + 1))

	fi
  done

  #Move unscheduled files to the top
  tail -n +$(expr $num + 1) $log_file > $init_list
  cat $tmp_log >> $init_list

  #Add recently added files to back of log
  cat $log_file | sort > /tmp/tmp.txt_old
  ls -l $BASE_PATH | grep -v total | awk '{print $NF}' | sort > /tmp/tmp.txt_new
  diff /tmp/tmp.txt_old /tmp/tmp.txt_new | grep "^>" | awk '{print $2}' >> $init_list
  mv $init_list $log_file

done
