#!/bin/bash

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : YYYYMMDD";
  exit ;
fi

YYYYMMDD=$1
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo  "NextDay: $next_working_day";
security_margin_file="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_$next_working_day.txt"
symbols="/home/pengine/prod/live_configs/nse_midterm_shortcodes"
symbols_1="/home/pengine/prod/live_configs/nse_midterm_shortcodes_1"

bank_val=`grep NSE_BANKNIFTY_P0_A $security_margin_file | tail -1  | awk '{print $2}'`
nifty_val=`grep NSE_NIFTY_P0_A $security_margin_file | tail -1  | awk '{print $2}'`

grep_count_=`grep NSE_BANKNIFTY_P0_A $security_margin_file | wc -l `
grep_count_2=`grep NSE_NIFTY_P0_A $security_margin_file | wc -l `
if [[ $grep_count_ -eq 0 ]] || [[ $grep_count_2  -eq 0 ]];then
	echo "ERROR :------------ NO ENTRY EXITING:-"
	echo "" | mailx -s "No Entry of NSE_BANKNIFTY_P0_A or NSE_NIFTY_P0_A in security margin file $next_working_day" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
	exit
fi

for shortcode in `cat $symbols $symbols_1 | grep 'NSE_NIFTY'`; do
  if ! grep -q -w $shortcode $security_margin_file ; then
    echo "$shortcode $bank_val"
    echo "$shortcode $bank_val" >>$security_margin_file
  fi
done
for shortcode in `cat $symbols $symbols_1 | grep 'NSE_BANKNIFTY'`; do
  if ! grep -q -w $shortcode $security_margin_file ; then
      echo "$shortcode $nifty_val"
      echo "$shortcode $nifty_val" >>$security_margin_file
  fi
done
