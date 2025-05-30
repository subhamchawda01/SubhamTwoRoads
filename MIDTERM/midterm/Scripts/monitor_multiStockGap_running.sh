#!/bin/bash


date_=`date +%Y%m%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
        echo "NSE Holiday. Exiting...";
        exit;
fi


count_=`ps aux | egrep "MultiStockGapHedger" | grep -v grep | wc -l`
echo "Current Count $count_"
if [[ $count_ -lt 1 ]]; then
      echo "Sending Mail for not running"
        echo "" | mailx -s "MultiStockGapHedger Not running" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, smit@tworoads-trading.co.in
fi

