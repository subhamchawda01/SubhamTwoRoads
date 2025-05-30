#!/bin/bash

today_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
     echo "NSE holiday..., exiting";
     exit
fi

echo "Giving start to INDB12"
for i in 200812 200813; do ssh 192.168.132.12  "/home/pengine/prod/live_execs/user_msg --traderid $i --start"; done
#echo "" | mailx -s "Start Given to Strats" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in 
