#!/bin/bash

HMGR="/home/pengine/prod/live_execs/holiday_manager"
UPDATE_DATE="/home/pengine/prod/live_execs/update_date"

running_date=""
current_day=$1

is_today_holiday=`$HMGR EXCHANGE CBOE $current_day T`

if [ "$is_today_holiday" != "2" ]; then
  running_date=`$UPDATE_DATE $current_day N W`
else
  running_date=$current_day
fi

echo "$running_date"
