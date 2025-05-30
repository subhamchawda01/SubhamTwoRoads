#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "USAGE <exec> <NAS path> <physical local path>";
fi
path=$1
local_path=$2
download_string="0"
log_file=$HOME/.ec2_cache_fetch_log
cmd="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o ConnectTimeout=5 10.0.0.11 cd /home/dvctrader/ec2_cache; check_presence_on_all_workers.sh $path 2> /dev/null"
#echo $cmd
download_string=`$cmd`
echo "Download str: "$download_string >> $log_file
#echo "Download str: "$download_string
if [ ${download_string:0:3} == "10." ]; then
  scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o ConnectTimeout=5 $download_string $local_path
  #echo "Saved file to: "$local_path >> $log_file
fi
