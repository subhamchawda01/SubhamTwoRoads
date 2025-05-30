#!/bin/bash

if [ $# -lt 1 ] ; then echo "$0 Normal(N)|Staged(S) [shc|ALL=ALL] [dump_loc=10.0.1.15] [output_dir=/NAS1/ec2_(staged_)globalresults] [start-date]"; exit 0; fi;

type=$1; shift;
shc="ALL";
if [ $# -ge 1 ] ; then shc=$1; shift; fi 
ip="10.0.1.15";
if [ $# -ge 1 ] ; then ip=$1; shift; fi 
num_days="400"
if [ $# -ge 1 ] ; then num_days=$1; shift; fi
start_date="$1"

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

echo "DUMPING...";
user="dvctrader";
ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=20 $user@$ip "/home/$user/basetrade/scripts/move_res_from_DB_to_FS.pl $dir $type 1 $shc $num_days $start_date";
sleep 2; #just a caution for FS to get updated before rsync, maybe helpful in case of NFS

echo -e "\nSYNCING...";
ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=20 $user@$ip "/home/$user/basetrade/scripts/sync_results_to_all_machines.sh $type $dir";
