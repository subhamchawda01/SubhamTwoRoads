#!/bin/bash


echo "Check_script_already_running"
running_=`ps aux | grep monitor_datacopy_slack.sh | grep -v grep  | wc -l`
echo "Running status $running_"
[[ $running_ -gt 0 ]] && { echo "script already Running"; exit;};


echo "" | mailx -s "monitor_datacopy_slack.sh script not running on 5.66" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in 
