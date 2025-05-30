#!/bin/bash

GetPreviousWorkingDay() {
      date=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
      is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
      while [ $is_holiday_ = "1" ];
      do
            date=`/home/pengine/prod/live_execs/update_date $date P A`
            is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
      done
}


send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
YYYYMMDD=`date +\%Y\%m\%d` ;
GetPreviousWorkingDay

tmp_file1="/tmp/margin_cal_read_current"
file_dump="/home/dvctrader/important/Margin_Util/margin_cal/$YYYYMMDD"

/home/dvctrader/anaconda3/bin/python /home/dvctrader/important/Margin_Util/margin_calc_v2.py --date $date --pos_file /spare/local/files/NSE/MidTermLogs/EODPosFiles/live --print_mode STRATWISE >$tmp_file1

time_=`date -d "+ 330 minutes" +'%H:%M:%S'`

echo >> $file_dump
echo "TIME:                                                $time_" >> $file_dump
cat $tmp_file1 >> $file_dump


margin=`cat $tmp_file1 | grep "Portfolio Margin:" | awk '{print $3}'`

echo "Current Margin:  $margin          Time: $time_"

[[ $margin -gt  '130000000' ]] && { echo "" | mailx -s "Midterm MARGIN ALERT ABOVE 13cr $margin $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in smit@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in ; }
#$send_slack_exec nseinfo DATA "CM Margin-$margin";}
