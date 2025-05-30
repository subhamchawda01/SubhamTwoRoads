#!/bin/bash

if [[ $# -eq 0 ]]
then
  today=`date +%Y%m%d`  
else
  today=$1
fi

get_yesterday () {
  this_day=$1
  day_of_week=`date -d "$this_day" +%w`
  if [[ $day_of_week == 1 ]]
  then
    look_back=3
  else
    look_back=1
  fi
  yesterday=`date -d "$this_day ${look_back} day ago" +%Y%m%d`
  echo $yesterday
}

end_date=$( get_yesterday $today )
start_date="20180112"
final_last_day_real_pnl=0
final_last_day_sim_pnl=0
cumulative_real_pnl=0
cumulative_sim_pnl=0
date=$end_date

while [[ $start_date -le $date ]];
do
  echo "Running for $date"
  yyyy=${date:0:4}
  mm=${date:4:2}
  dd=${date:6:2}

  file_format="/NAS1/logs/QueryTrades/"$yyyy"/"$mm"/"$dd"/trades."$date".5348*"
  real_pnl_day=0
  for i in `ls $file_format`
  do
    pnl_day=`tail -n1 $i | awk '{print $18}'`
    pnl_day=${pnl_day%.*}
    real_pnl_day=$(($real_pnl_day+$pnl_day))
  done;
  cmd="python ~/basetrade/RiskAllocation/allocate_risk_V1.py -fl ~/dvccode/configs/mrt_all_strats -sd $date -ed $date -lb 0 -ub 1 -rf 1 -bl 2 -file_write 0 | grep RealPortPnl | awk '{print \$2}'"
#echo $cmd
  sim_pnl_day=$(eval $cmd)
  sim_pnl_day=${sim_pnl_day%.*}
  
#### Manual Offsetting for some days ####
  if [[ $date -eq "20180117" ]]
  then
    real_pnl_day=$(($real_pnl_day+2901))
  fi

#### Logging data over here #######
  echo "Real Pnl: "$real_pnl_day
  echo "Sim Pnl: "$sim_pnl_day
  

#### Finally assigning values #####
  if [[ $date -eq $end_date ]]
  then
    final_last_day_real_pnl=$real_pnl_day
    final_last_day_sim_pnl=$sim_pnl_day
  fi
  
  cumulative_real_pnl=$(($cumulative_real_pnl+$real_pnl_day))
  cumulative_sim_pnl=$(($cumulative_sim_pnl+$sim_pnl_day))

  date=$( get_yesterday $date )

done;

mail_body=""
mail_body=$mail_body"For "$end_date":\n"
mail_body=$mail_body"Real Portfolio Pnl = "$final_last_day_real_pnl"\n"
mail_body=$mail_body"Sim Portfolio Pnl = "$final_last_day_sim_pnl"\n";
mail_body=$mail_body"Since "$start_date":\n"
mail_body=$mail_body"Cumulative Real Portfolio Pnl = "$cumulative_real_pnl"\n"
mail_body=$mail_body"Sim Portfolio Pnl = "$cumulative_sim_pnl"\n";

printf "$mail_body" | mail -s "SimReal MRT Portfolio $end_date" mehul.goyal@tworoads.co.in nseall@tworoads.co.in
