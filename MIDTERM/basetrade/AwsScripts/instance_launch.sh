#!/bin/bash
source ~/.bashrc

WORK_DIR="/home/dvctrader/controller_scripts"

LOCK_FILE=$WORK_DIR"/START_INST_LOCK"
LOG_FILE=$WORK_DIR"/START_INST_LOG"

INSTANCE_TYPE="m1.large"
AMI_ID="ami-7a89c813"
DATA_FILE="/home/dvctrader/controller_scripts/ec2_startup_instructions.sh"

(
  flock -x -w 1 200 || exit 1  #lock for 5 minutes
  IP=` ec2-run-instances $AMI_ID -t $INSTANCE_TYPE -k dvckeypair_virginia -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY -f $DATA_FILE -s subnet-c6d5bba8 | grep PRIVATEIPADDRESS | awk '{print $2}' `
  NUM_TRIES=100
  # try 100 times after every 3 seconds to ping the machine. EBS volumes are typically ready in 60 secs as per AWS docs
  if [[ $IP =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
  for i in ` seq 1 $NUM_TRIES` 
    do 
      CAN_REACH=`ping -i 1 -c 1 -W 3 $IP  | grep -c icmp_seq`
      if [ "$CAN_REACH" == "1" ]; then 
        echo $IP" ping successful"
        break; 
      else
        echo "cant reach "$IP".. retrying"
      fi
    done
  else
    echo "invalid ip received: "$IP
  fi
) 200>$LOCK_FILE


