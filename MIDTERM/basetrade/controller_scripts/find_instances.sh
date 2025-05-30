#!/bin/bash
source ~/.bashrc

#Output the instance-id running/stopped-status public-ip

ec2-describe-instances -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY --show-empty-fields | grep "^INSTANCE" | awk '{print $2 , $6, $17, $18}'

