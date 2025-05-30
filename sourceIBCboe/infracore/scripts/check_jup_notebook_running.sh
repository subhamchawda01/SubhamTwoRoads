#!/bin/bash

count_=`ps aux | egrep "jupyter|ssh -f -X" | grep -v grep | wc -l`
echo "Current Count $count_"
if [[ $count_ -lt 2 ]]; then
  echo "Sending Mail for not running"
  echo "" | mailx -s "Jupyter NoteBook Not running" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in 
  ssh root@10.23.5.67 "/home/pengine/prod/live_scripts/dvc_jupyter_daemon.sh"
fi
