#!/bin/bash

if [ $@ > 0 ] ; then
IP=$1
else
IP="."
fi

uid_list=`ls /mnt/sdf/JOBS/job_desc/COMMAND* | grep $IP | awk -F "_" '{ for (i=4; i<NF;i=i+1){printf ("%s_", $i)} print $NF}'`
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
  
  if [[ $num_words -eq "2" ]] ; then
     #d=`date -d $start_time`   
     echo $ip" "$start_time" : "$cmd_line
  fi
done
