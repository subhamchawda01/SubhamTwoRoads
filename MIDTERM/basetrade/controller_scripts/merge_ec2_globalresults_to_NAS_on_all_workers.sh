#!/bin/bash

#ip_list=`cat ~/AWSScheduler/instance_cores.txt | awk '{if($2>0){ print $1; }}'`;
ip_list=`cat /mnt/sdf/JOBS/all_instances.txt | grep nil | awk '{print $4}'`;

for ip in $ip_list; do echo $ip; ssh $ip '/home/dvctrader/basetrade_install/scripts/sync_ec2_globalresults_NAS.pl /mnt/sdf/ec2_globalresults /NAS1/ec2_globalresults' >~/merge_ec2_log_"$ip" 2>&1 & done 
