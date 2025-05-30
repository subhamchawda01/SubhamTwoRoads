#!/bin/bash

YYYYMMDD=`date +"%Y%m%d"`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

count=`ps aux | grep -v 'grep' | grep NSEShmWriter | wc -l`
echo $count 
i=0
while [[ $count -lt 2 ]]; do
      echo "RESTARTING NSESHM WRITER "
    /home/pengine/prod/live_scripts/SmartDaemonController.sh NSEShmWriter STOP
    /home/pengine/prod/live_scripts/SmartDaemonController.sh NSEShmWriter START
    sleep 1m;
    count=`ps aux | grep -v 'grep' | grep NSEShmWriter | wc -l`
    i=$(($i + 1))
    [[ $i -gt 5 ]] && break;
done
