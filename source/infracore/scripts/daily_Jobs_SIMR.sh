#!/bin/bash

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 YYYYMMDD" ;
  exit ;
fi
date=$1
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
      exit;
fi
next_working_day=`/home/pengine/prod/live_execs/update_date $date N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]]
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done


echo "Generate UnAdjusted Data"
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/BTtools/Data_Generation/GenerateUnadjData_v2.py --startdate 20180101

cd /spare/local/DataLib/NSE_FutData/Unadjusted 
count_=`grep $date SBIN.csv HDFCBANK.csv  TATASTEEL.csv  | wc -l`
echo "Unadjusted Count: $count_"
if [[ $count_ -lt 3 ]] ; then
    echo "Algo Count Missing "
  echo "" | mailx -s "SIMR Unadjust Count Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
  exit
fi

echo "Generate Adjusted Data"
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/BTtools/Data_Generation/GenerateAdjData.py
echo "GenerateBarDataFromAdjMinuteData"
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/BTtools/Data_Generation/GenerateBarDataFromAdjMinuteData.py
echo "Run IndicatorData Parallel"
/apps/anaconda/anaconda3/envs/py3/bin/python  /home/dvctrader/MFT_Analysis/Analysis/ML/GenerateIndicatorData_parallel.py

count_=`grep $date '/spare/local/DataLib/Daily Universe/Daily Universes.csv' | wc -l`
echo "Daily Universe Count: $count_"
if [[ $count_ -lt 1 ]] ; then
  echo "SIMR Universe Missing "
  echo "" | mailx -s "SIMR Universe Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
  exit
fi

echo "Syncing SIMR"
#rsync -avz '/spare/local/DataLib/Daily Universe/' 10.23.227.69:/home/dvctrader/ATHENA/SIMR_LIVE
rsync -avz '/spare/local/DataLib/Daily Universe' 52.90.0.239:/spare/local/DataLib
rsync -avz '/home/dvctrader/AnalysisOut/ML/Indicator Pickle Dumps/' 52.90.0.239:"/home/dvctrader/rahul/simr_tradeengine/IndicatorDump"
#rsync -avz '/home/dvctrader/AnalysisOut/ML/Indicator Pickle Dumps/' 10.23.227.69:/home/dvctrader/ATHENA/SIMR_LIVE

echo "/home/dvctrader/venv/bin/python3 /home/dvctrader/rahul/simr_tradeengine/simr_assetCreator.py $next_working_day $next_working_day"
ssh 52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/rahul/simr_tradeengine/simr_assetCreator.py $next_working_day $next_working_day"

rsync -avz 52.90.0.239:/home/dvctrader/rahul/simr_tradeengine/CONFIG_FUT0/SIMR_CONFIG /home/dvctrader/ATHENA
rsync -avz /home/dvctrader/ATHENA/SIMR_CONFIG 10.23.227.69:/home/dvctrader/ATHENA/SIMR_LIVE/CONFIG_FUT0/

simrOut="/home/dvctrader/ATHENA/SIMR_CONFIG/$next_working_day"
echo "SIMR FILE to Check: $simrOut"

count=`grep SIGMA_ $simrOut | wc -l`
count2=`grep MARGIN_ $simrOut | wc -l`
count3=`grep ASSET_SHORT $simrOut | wc -l`
echo "SIGMA Count: $count MARGIN: $count2 ASSET SHORT: $count3"
if [[ $count -lt 1 || $count2 -lt 1 || $count3 -lt 1 ]] ; then
    echo "SIMR COUNT MISSING: sending mail"
  echo "SIGMA Count: $count MARGIN: $count2 ASSET SHORT: $count3" | mailx -s "SIMR COUNT MISSING FORDATE : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
  exit
fi

echo "" | mailx -s "SIMR Universe Updated : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
