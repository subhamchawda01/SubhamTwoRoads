#!/bin/bash

prod_remove=(YESBANK)

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : " $* "Date";
  exit ;
  exit
fi
  
YYYYMMDD=$1;  
next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done

echo  "NextDay: $next_working_day";
security_margin="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_$next_working_day.txt"
cp $security_margin /home/dvctrader/trash/
removed_symbols="/home/dvctrader/important/removing_symbols"
tmp_file="/tmp/dummy_security_margin.txt"

printf "%s\n" "${prod_remove[@]}" > $removed_symbols
grep -v -f $removed_symbols $security_margin >$tmp_file
mv $tmp_file $security_margin
chgrp infra $security_margin
