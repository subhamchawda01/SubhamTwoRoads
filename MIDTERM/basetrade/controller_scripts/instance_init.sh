#!/bin/bash
source ~/.bashrc

instance_id=$1 
cmd=$2

case "$cmd" in
  START)
    ec2-start-instances $instance_id -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY 
    NUM_TRIES=100
    IP=`grep $instance_id /mnt/sdf/JOBS/all_instances.txt | awk '{print $4}'`
    echo "trying to install initial scripts on started instance"
    for i in ` seq 1 $NUM_TRIES`
    do
      CAN_REACH=`ping -i 1 -c 1 -W 3 $IP  | grep -c icmp_seq`
      if [ "$CAN_REACH" == "1" ]; then
        echo $IP" ping successful"
        echo "sleep for 60 secs before executing user scripts"
        sleep 60
        sh /home/dvctrader/controller_scripts/redo_stratup_script_on_worker.sh $IP
        break;
      else
        echo "cant reach "$IP".. retrying"
      fi
    done

  ;;
  STOP)
    ec2-stop-instances $instance_id -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY 
  ;;
  TERMINATE)
    ec2-terminate-instances $instance_id -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY 
  ;;
  IS_RUNNING)
    ec2-describe-instances -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY --show-empty-fields | grep "^INSTANCE" | grep $instance_id | awk '{ if( $6 == "stopped" ) { print "FALSE"; } else { print "TRUE"; } }' 
  ;;
  STATUS)
    ec2-describe-instance-status $instance_id -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY
  ;;
  *)
  ;;
esac
  
