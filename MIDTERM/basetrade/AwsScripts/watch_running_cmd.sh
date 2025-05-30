#!/bin/bash

uid_list=`ls /mnt/sdf/JOBS/job_desc/COMMAND* | awk -F "_" '{ for (i=4; i<NF;i=i+1){printf ("%s_", $i)} print $NF}'`

for uid in $uid_list ; 
do 
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
      mail -s "$cmd_line" dvccronjobs@circulumvite.com < /mnt/sdf/JOBS/log/log*$uid # switched to dvccronjobs from dvctrader recently
    fi
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
