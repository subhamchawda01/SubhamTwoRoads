#!/bin/bash

date=`date +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
       echo "NSE Holiday. Exiting...";
       exit;
fi

GetPreviousWorkingDay() {
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
    while [ $is_holiday_ = "1" ];
    do
        YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
        is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
    done
}
 
YYYYMMDD=$date
GetPreviousWorkingDay

echo "$YYYYMMDD" 


file="/spare/local/files/NSE/MidTermLogs/Betas/$YYYYMMDD"
echo $file
if [ ! -s "$file" ]
then 
    echo "File not exist"
    echo "" | mailx -s "IND12 Betas file missing for $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  smit@tworoads-trading.co.in;
fi

