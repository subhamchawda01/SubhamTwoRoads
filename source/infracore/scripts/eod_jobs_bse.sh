#!/bin/bash

today=`date +"%Y%m%d"`;

if [ $# -eq 1 ];
then
    today=$1;
fi

i=0;
echo "Start: $today" >>/spare/local/files/eod_complete_bse.txt

echo "Start Ratio: CASH BSE_SBIN_NSE_SBIN"
echo "/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 31 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_AI.csv 1>/spare/local/logs/log_calc_ratio_start_cm_bse  2>&1 & "
/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 31 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_AI.csv 1>/spare/local/logs/log_calc_ratio_start_cm_bse  2>&1 &
pids[i]=$!;
i=${i+1};

echo "Start Ratio: CASH BSE_SBIN_NSE_SBIN"
echo "/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 32 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_JZ.csv 1>/spare/local/logs/log_calc_ratio_start_cm_bse  2>&1 & "
/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 32 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_JZ.csv 1>/spare/local/logs/log_calc_ratio_start_cm_bse  2>&1 &
pids[i]=$!;
i=${i+1};

echo "End Ratio: FUT1 NSE_SBIN_FUT1_NSE_SBIN_FUT0"
echo "/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 33 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_AI.csv 1>/spare/local/logs/log_calc_ratio_end_cm_bse 2>&1 & "
/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 33 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_AI.csv 1>/spare/local/logs/log_calc_ratio_end_cm_bse 2>&1 &
pids[i]=$!;
i=${i+1};


echo "End Ratio: FUT1 NSE_SBIN_FUT1_NSE_SBIN_FUT0"
echo "/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 34 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_JZ.csv 1>/spare/local/logs/log_calc_ratio_end_cm_bse 2>&1 & "
/home/dvctrader/stable_exec/scripts/calc_ratio_bse.sh $today 34 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_BSE_CM_RATIO_CALCULATOR/LIVE_FILE_JZ.csv 1>/spare/local/logs/log_calc_ratio_end_cm_bse 2>&1 &
pids[i]=$!;
i=${i+1};


 
pids[i]=$!;
i=${i+1};
for pid in ${pids[*]}; do
    wait $pid
done

#Create a entry of complete so that other scripts can run
echo "END: $today" >>/spare/local/files/eod_complete_bse.txt
