#!/bin/bash

LOCK_FILE=$HOME/AWSScheduler/.mail_agg_lockfile
if [ -f $LOCK_FILE ]
then
  echo "Aggregator Lock file $LOCK_FILE exists. Can't start process";
  exit 1;
fi
touch $LOCK_FILE
trap "rm -f $LOCK_FILE" EXIT

#d=`date +'%s'` ; ~/controller_scripts/watch_running_only.sh  | sed 's/>.*//' | awk -v d=$d '{$1=d-$2}1' | sort  -nr

#uid_list=`ls /mnt/sdf/JOBS/job_desc/COMMAND* | awk -F "_" '{ for (i=4; i<NF;i=i+1){printf ("%s_", $i)} print $NF}'`

d=`date +'%s'`
> /mnt/sdf/JOBS/emails/cleanup_agg_email.txt
for line in ` ls /mnt/sdf/JOBS/job_desc/COMMAND*` ; 
do 
  uid=`echo $line | awk -F "_" '{ for (i=4; i<NF;i=i+1){printf ("%s_", $i)} print $NF}'`
  ip=`echo $line | awk -F "_" '{print $3}'`

  cmd_line=`cat /mnt/sdf/JOBS/job_desc/COMMAND*$uid | sed -ne '2p' `
  time_list=`stat -c%Y /mnt/sdf/JOBS/job_desc/*$uid | sort -n ` ; 
  num_words=`echo $time_list | wc -w `
  start_time=`echo $time_list | awk '{print $1}'`
  end_time=`echo $time_list | awk '{print $3}'`

  #echo  "num_words "$num_words" uid "$uid
  
  if [[ $num_words -eq "3" ]] ; then
    echo $uid "DONE"; 
    echo $uid " " $start_time " " $end_time " " $cmd_line >> /mnt/sdf/JOBS/done_q.txt
    if [ -s /mnt/sdf/JOBS/log/log*$uid ];
    then
      echo "START" >> /mnt/sdf/JOBS/emails/cleanup_agg_email.txt
      echo "\"$cmd_line\"" >> /mnt/sdf/JOBS/emails/cleanup_agg_email.txt
      cp /mnt/sdf/JOBS/log/log*$uid /mnt/sdf/JOBS/emails/
      ls /mnt/sdf/JOBS/emails/log*$uid >> /mnt/sdf/JOBS/emails/cleanup_agg_email.txt
      echo "END" >> /mnt/sdf/JOBS/emails/cleanup_agg_email.txt
    fi
    rm -f /mnt/sdf/JOBS/log/log*$uid
    rm /mnt/sdf/JOBS/job_desc/*$uid
  elif [[ $num_words -eq "2" ]] ; then
      already_added=`grep -c $uid /mnt/sdf/JOBS/running_q.txt` ;
      if [[ $already_added -eq "0" ]];
      then
        echo $uid" added to running file"
        echo $uid" " $start_time " XXXX " $cmd_line >> /mnt/sdf/JOBS/running_q.txt
      fi
  elif [[ $num_words -eq "1" ]] ; then
      echo $uid " unassigned_command.. please look"; 
      echo $cmd_line >> /mnt/sdf/JOBS/failed_q.txt
      rm  /mnt/sdf/JOBS/job_desc/*$uid
  else 
      echo "error case "$uid " file_count "$num_words 
  fi
done

~/controller_scripts/generate_and_send_agg_email_with_log.pl

rm -f $LOCK_FILE
