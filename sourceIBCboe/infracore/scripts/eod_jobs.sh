#!/bin/bash

today=`date +"%Y%m%d"`;

if [ $# -eq 1 ];
then
    today=$1;
fi

i=0;


/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 10 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT1_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_start_fut1  2>/spare/local/logs/log_calc_ratio_start_fut1 & 
pids[i]=$!;
i=${i+1};

/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 11 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT1_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_end_fut1 2>/spare/local/logs/log_calc_ratio_end_fut1 & 
pids[i]=$!;
i=${i+1};
 
/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 12 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT2_RATIO_CALCULATOR/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_start_fut2 2>/home/dvctrader/usarraf/log_calc_ratio_start_fut2 & 
pids[i]=$!;
i=${i+1};

/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 13 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT2_RATIO_CALCULATOR/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_end_fut2 2>/home/dvctrader/usarraf/log_calc_ratio_end_fut2 & 
pids[i]=$!;
i=${i+1};

/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 14 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_start_cm1  2>/spare/local/logs/log_calc_ratio_start_cm1 & 
pids[i]=$!;
i=${i+1};

/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 15 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_end_cm1 2>/spare/local/logs/log_calc_ratio_end_cm1 & 
pids[i]=$!;
i=${i+1};

load_worker=`uptime | awk '{print $12}' | cut -d'.' -f1`
echo "Current Load on worker load_worker $load_worker";
if [[ $load_worker -gt 5 ]]; then
	echo "Waiting For 20mins "
	sleep 1m;
fi
echo "Running BarData Task"

/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 16 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR_FUT1/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_start_cm2 2>/home/dvctrader/usarraf/log_calc_ratio_start_cm2 & 
pids[i]=$!;
i=${i+1};

/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 17 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR_FUT1/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_end_cm2 2>/home/dvctrader/usarraf/log_calc_ratio_end_cm2 & 
pids[i]=$!;
i=${i+1};

#/home/dvctrader/stable_exec/fut_data_gen.sh $today >/tmp/bardatagen.log

/home/dvctrader/stable_exec/scripts/generate_bar_data.sh $today ~/RESULTS_FRAMEWORK/strats/prod_file_extended 20 NON 1>/spare/local/logs/log_momentum_adj  2>/spare/local/logs/log_momentum_adj

/home/dvctrader/stable_exec/scripts/generate_bar_data_tuesday.sh $today ~/RESULTS_FRAMEWORK/strats/prod_file_extended_tuesday 20 NON 1>/spare/local/logs/log_momentum_adj_fin  2>&1

#FOR ADJUST WED EXPRIY WEEK ~/EOD_SCRIPTS/on_expiry.sh

pids[i]=$!;
i=${i+1};
for pid in ${pids[*]}; do
    wait $pid
done

#Create a entry of complete so that other scripts can run
echo "$today" >>/spare/local/files/eod_complete.txt
