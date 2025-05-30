#!/bin/bash
if [ $# -lt 1 ]; then
  echo "$0 LOCATION"
  exit 0
fi

#Location
location=$1

#Can be any EC2 worker machine
trigger_worker="10.0.1.15"
#In case original worker fails
fallback_worker="10.0.1.77"

if [ "$2" == "TODAY" ]; then
  sleep 300
fi

num_retries=0
while [ true ]; do
  if [ "$num_retries" -gt 2 ]; then
    echo "Already tried max possible times. Exiting"
    exit 0
  fi

  worker_load=`ssh dvctrader@$trigger_worker uptime | tr , ' ' | head -1 | awk '{max_load=$11; printf "%d\n",max_load;}'`
  if [ $? -ne 0 ] || [ "$worker_load" == "" ] || [ $worker_load -gt 32 ]; then
    echo "Falling back to $fallback_worker"
    trigger_worker=$fallback_worker
  fi

  ssh dvctrader@$trigger_worker "bash /media/disk-ephemeral2/command_files/sampledata_procs/sd_procs_$location"
  
  num_retries=$(($num_retries+1))
  sleep 300
done

