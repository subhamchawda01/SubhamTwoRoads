#!/bin/bash

last_date_=2021302

start_date_=20210302
echo $line
while [[ $start_date_ -lt $last_date_ ]] ; do
        echo "./update_db_daily_eod.sh $start_date_" 
       ./update_db_daily_eod.sh $start_date_
        start_date_=`/home/pengine/prod/live_execs/update_date $start_date_ N A`
        is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $start_date_ T`
        while [[ $is_holiday = "1" ]] 
        do
                start_date_=`/home/pengine/prod/live_execs/update_date $start_date_ N A`
                is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $start_date_ T`
        done
done

