#!/bin/bash

date=`date +%Y%m%d`
echo "date:: $date"
is_pos_exec_running=`ps aux | grep "/home/dvctrader/ATHENA/post_market_exec pos_exec_file_temp" | grep $date | wc -l`;

echo "ans = $is_pos_exec_running";

if [ "$is_pos_exec_running" -eq "1" ]; 
then 
  echo "exec is still running";
  echo -e "pre_market exec is Still Running" | mail -s "***Pre_Market exec Alert***:: $date" hardik.dhakate@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads-trading.co.in uttkarsh.sarraf@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in 
#else 
  #echo "NA"; 
  #echo -e "No Issues of pre_market exec" | mail -s "***Pre_Market exec Alert***:: $date" hardik.dhakate@tworoads-trading.co.in 
fi
