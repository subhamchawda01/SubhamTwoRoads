#!/bin/bash

GetNearestExpiry() {
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${today}
  expiry=`cat ${contract_file} | grep IDX | grep $shortcode | awk -v date=${today} '{if($6>=date)print $6'} | sort | uniq | head -n1`
}
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'


if [ $# -ne 2 ] ;
then
    echo script DATE
    exit;
fi

today=$1
Check_=$2
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

folder_to_sync="CONFIG_OPT_IDX_STRANGLE_ALL_RUSH"
shortcode="BANKNIFTY"
GetNearestExpiry
if [[ $today == $expiry ]]; then
    folder_to_sync="CONFIG_OPT_IDX_STRANGLE_ALL_EXPIRY_RUSH"
fi


check_copy_running=`ps aux | grep copy_straddle_files_from_worker | grep -v grep | wc -l`
echo "Check Copy Running $check_copy_running"
if [[ $check_copy_running -gt 0 ]]; then
  
  echo "Copy Running From Ind20 $check_copy_running"
  echo "" | mailx -s "Not Running Straddle For $today, Copy Status $check_copy_running" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in
  exit
fi

echo "Sync Files of: $today To Worker "
#next_working_day=`/home/pengine/prod/live_execs/update_date $today N A`
while true; do
#  echo "Sycning the File from IND20"
#  rsync -avz dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_OPT_IDX_ATM_STRADDLE_SPOT_IBB_HDG_LONGSPLIT /home/dvcinfra/important/
####  rsync -avz dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_OPT_IDX_STRANGLE_ALL /home/dvcinfra/important/
  echo "rsync -avz /home/dvcinfra/important/STRADDLE_FILES/$today/$folder_to_sync/ dvctrader@52.90.0.239:/home/dvctrader/nishit/SIM_CHECK_CONFIG_OPT_IDX_STRANGLE/"
  rsync -avz /home/dvcinfra/important/STRADDLE_FILES/$today/$folder_to_sync/ dvctrader@52.90.0.239:/home/dvctrader/nishit/SIM_CHECK_CONFIG_OPT_IDX_STRANGLE/
  status1=$?
  [ $status1 -ne 0 ] && { echo "Sync Failed to Worker Retrying..."; sleep 1m; continue;  }
  break;
done
echo "Check For dataCopy"
data_copy_running=`ps aux | egrep  -i "check_nse_data_sync|SyncOptionsDay" | grep -v 'grep' | grep -v 'tail' | grep -v 'vim' | grep -v 'less' | wc -l`
echo "DataCopy Status: $data_copy_running"
if [[ $Check_ == "N" ]]; then
  echo "No Check For DATACOPY"
  data_copy_running=0
fi

while [[ $data_copy_running -gt 0 ]]; do 
  sleep 10;
  data_copy_running=`ps aux | egrep  -i "check_nse_data_sync|SyncOptionsDay" | grep -v 'grep' | grep -v 'tail' | grep -v 'vim' | grep -v 'less' | wc -l`
  echo "DATACOPY Status: $data_copy_running"
done 

echo "Check Ram Available To Run"

mem=`ssh dvctrader@52.90.0.239 "free -h" | grep Mem | awk '{print $NF}'| cut -d'G' -f1`;
echo "Worker Free Mem: $mem"
i=0
while [[ $mem -lt 27 ]];
do
  sleep 60;
  mem=`ssh dvctrader@52.90.0.239 "free -h" | grep Mem | awk '{print $NF}'| cut -d'G' -f1`
  echo "Worker Free Mem: $mem"
  i=$((i+1))
  [[ $i -gt 120 ]] && break;
done

echo "Running Straddle with Current Memory $mem For $today" 
ssh dvctrader@52.90.0.239 "/home/dvctrader/stable_exec/straddle_daily_jobs_20220201.sh $today";
sleep 60;
Instance=`ssh dvctrader@52.90.0.239 "ps aux | egrep 'straddle_daily_jobs|trade_engine_idx_' | grep 12355 | grep -v grep" | wc -l`
echo "Instance: $Instance"
i=0
while [[ $Instance -gt 0 ]];
do
  sleep 180;
  Instance=`ssh dvctrader@52.90.0.239 "ps aux | egrep 'straddle_daily_jobs|trade_engine_idx_' | grep 12355 | grep -v grep" | wc -l`
  echo "STRADDLE Status: $Instance"
  i=$((i+1))
  [[ $i -gt 500 ]] && break;
done

echo "Done Wait for 1min"
sleep 1m


ind20_pnl=`ssh dvctrader@10.23.227.71 "zgrep 'PORTFOLIO PNL' /spare/local/logs/tradelogs/log.${today}.123552.gz  | tail -n1 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
ind20_pnl_2=`ssh dvctrader@10.23.227.84 "zgrep 'PORTFOLIO PNL' /spare/local/logs/tradelogs/log.${today}.123551.gz  | tail -n1 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
ind20_pnl=$(($ind20_pnl + $ind20_pnl_2)) 

echo "$ind20_pnl"
strat_1=`ssh dvctrader@52.90.0.239 "tail /spare/local/logs/tradelogs/log.${today}.1235521 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
strat_2=`ssh dvctrader@52.90.0.239 "tail /spare/local/logs/tradelogs/log.${today}.1235522 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
strat_3=`ssh dvctrader@52.90.0.239 "tail /spare/local/logs/tradelogs/log.${today}.1235523 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
strat_4=`ssh dvctrader@52.90.0.239 "tail /spare/local/logs/tradelogs/log.${today}.1235524 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
strat_5=`ssh dvctrader@52.90.0.239 "tail /spare/local/logs/tradelogs/log.${today}.1235525 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
strat_6=`ssh dvctrader@52.90.0.239 "tail /spare/local/logs/tradelogs/log.${today}.1235526 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`
strat_7=`ssh dvctrader@52.90.0.239 "tail /spare/local/logs/tradelogs/log.${today}.1235527 | grep 'PORTFOLIO PNL'"| awk '{print $3}'`

echo "$today $ind20_pnl $strat_1 $strat_2 $strat_3 $strat_4 $strat_5 $strat_6 $strat_7" >>/spare/local/files/straddle_results.txt
rsync -avz /spare/local/files/straddle_results.txt  dvctrader@52.90.0.239:/spare/local/files/straddle_results.txt
#echo "IND20: $ind20_pnl 1235521: $strat_1 1235522: $strat_2 1235523: $strat_3 1235524: $strat_4 1235525: $strat_5 1235526: $strat_6" | mailx -s "Straddle Calculated For $today" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
#exit

echo "IND20: $ind20_pnl 1235521: $strat_1 1235522: $strat_2 1235523: $strat_3 1235524: $strat_4 1235525: $strat_5 1235526: $strat_6 1235527: $strat_7" | mailx -s "Straddle Calculated For $today" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" nishit.bhandari@tworoads.co.in, raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in, arpit.agarwal@tworoads-trading.co.in 

count_1=`ssh dvctrader@52.90.0.239 "grep 'CASC' /spare/local/logs/tradelogs/log.${today}.12355* | grep -w '0.000000' | grep -v 'UNCASCADE' | grep -v 'Percent'" | wc -l`
if [[ $count_1 -gt 0 ]]; then
      echo "" | mailx -s "Straddle Issue Please check For $today" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in 
fi

echo "Running Long Straddle"

/home/pengine/prod/live_scripts/run_long_straddle_on_worker.sh $today

