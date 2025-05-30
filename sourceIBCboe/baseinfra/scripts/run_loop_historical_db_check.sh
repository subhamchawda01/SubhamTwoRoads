#!/bin/bash

today_=`date +%Y%m%d`
date_=20210101
mailfile=/tmp/data_db_report.html_temp_txt
>$mailfile
echo "file check $mailfile"
echo ""
echo ""
while [[ $date_ -lt $today_ ]] ; do
        echo "checking for$date_"
        echo "./db_jobs_complete_only_check_local.sh $date_"
        ./db_jobs_complete_only_check_local.sh $date_

        date_=`/home/pengine/prod/live_execs/update_date $date_ N A`
        is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
        while [[ $is_holiday = "1" ]]
        do
                date_=`/home/pengine/prod/live_execs/update_date $date_ N A`
                is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
        done
done


