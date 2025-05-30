#!/bin/bash

date_=`date +"%Y%m%d"`

posfile="/spare/local/files/NSE/MidTermLogs/EODPosFiles/$date_"

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
    echo "NSE Holiday. Exiting...";
    exit;
fi
echo "Check file"      
echo $posfile
if [ ! -s "$posfile" ]
then
        echo "File not exist"
            echo "" | mailx -s "Error: IND12 EODPosFile for $date Not generated" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  smit@tworoads-trading.co.in  subham.chawda@tworoads-trading.co.in ;
fi

