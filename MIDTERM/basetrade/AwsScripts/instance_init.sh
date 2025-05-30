#!/bin/bash
source ~/.bashrc

instance_id=$1 
cmd=$2

case "$cmd" in
  START)
    ec2-start-instances $instance_id -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY 
  ;;
  STOP)
    ec2-stop-instances $instance_id -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY 
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
  
