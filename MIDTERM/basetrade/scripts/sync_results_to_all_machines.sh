#!/bin/bash

if [ $# -lt 1 ] ; then echo "$0 Normal(N)|Staged(S) [output_dir=/NAS1/ec2_(staged_)globalresults]"; exit 0; fi;

type=$1; shift;
case $type in 
  N)
    dir="/NAS1/ec2_globalresults";
    ;;
  S)
    dir="/NAS1/ec2_staged_globalresults";
    ;;
  *)
    echo "Type should be N|S"; 
    exit 0;
    ;;
esac
if [ $# -ge 1 ] ; then dir=$1; shift; fi 

user="dvctrader";
SSH_TIMEOUT=60;
worker_list=$(ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=$SSH_TIMEOUT $user@10.0.0.11 cat /mnt/sdf/JOBS/all_instances.txt | grep nil | awk '{print $4}');
echo "Woker List:", $worker_list;
case $type in 
  N)
    echo "Syncing to NY11";
    rsync -raz --timeout=$SSH_TIMEOUT $dir/ $user@10.23.74.51:~/ec2_globalresults >/dev/null;
    for worker in $worker_list ;
    do 
      echo "Syncing to Worker", $worker;
      rsync -raz --timeout=$SSH_TIMEOUT -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" $dir/ $user@$worker:/NAS1/ec2_globalresults >/dev/null ;
    done
    ;;
  S)
    echo "Syncing to NY11";
    rsync -raz --timeout=$SSH_TIMEOUT $dir/ $user@10.23.74.51:~/ec2_staged_globalresults >/dev/null;
    for worker in $worker_list ;
    do
      echo "Syncing to Worker", $worker;
      rsync -raz --timeout=$SSH_TIMEOUT -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" $dir/ $user@$worker:/NAS1/ec2_staged_globalresults >/dev/null ;
    done
    ;;
  *)
    echo "Type should be N|S"; 
    exit 0;
    ;;
esac

