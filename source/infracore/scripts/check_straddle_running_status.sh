#!/bin/bash

if [ $# -ne 1 ] ;
then
    echo script DATE
    exit;
fi


today=$1
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

Instance=`ssh dvctrader@52.90.0.239 "ps aux | egrep 'straddle_daily_jobs|trade_engine_idx_' | grep 12355 | grep -v grep" | wc -l`
echo "Instance: $Instance"
if [[ $Instance -gt 0 ]]; then
  echo "Seems Fine exiting..."
  exit
fi
echo "Error Sending Alert"
echo ""| mailx -s "Straddle Not Running On Worker. Please check as No Instance Found: $Instance" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in 

