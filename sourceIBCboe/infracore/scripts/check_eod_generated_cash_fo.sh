#!/bin/bash

if [ "$#" -ne 1 ] ; then
  echo "USAGE: SCRIPT <CM/FO>"
  exit
fi

dt=`date +%Y%m%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $dt T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

if [[ $1 == "CM" ]]; then
        echo "Checking for CM"
        file="/NAS1/data/MFGlobalTrades/ind_pnls/CM/eod_positions/ind_pos_${dt}.txt"
        file_2="/NAS1/data/MFGlobalTrades/ind_pnls/CM/eod_positions/ind_intraday_pos_${dt}.txt"
        file_3="/NAS1/data/MFGlobalTrades/ind_pnls/CM/eod_pnls/ind_pnl_${dt}.txt"
        echo "$file $file_2 $file_3"
#        if [ ! -s "$file"  ] || [ ! -s "$file_2" ] || [ ! -s "$file_3" ] ;
        if [ ! -s "$file_3"  ]
        then
                echo "File not exist"
                echo "" | mailx -s "Error: CM EODPosFile for $date Not generated" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in ravi.parikh@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in;
        fi

elif [[ $1 == "FO" ]]; then
        echo "Checking for FO"
        file="/NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_positions/ind_intraday_pos_${dt}.txt"
        file_2="/NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_positions/ind_pos_${dt}.txt"
        file_3="/NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${dt}.txt"
        echo "$file $file_2 $file_3"
        if [ ! -s "$file"  ] || [ ! -s "$file_2" ] || [ ! -s "$file_3" ] ;
        then
                echo "File not exist"
                echo "" | mailx -s "Error: FO EODPosFile for $date Not generated" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in ravi.parikh@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in;
        fi

fi
