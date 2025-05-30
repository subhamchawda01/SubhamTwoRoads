#!/bin/bash

today=`date +"%Y%m%d"`;

if [ $# -eq 1 ];
then
    today=$1;
fi

i=0;

echo "BarStart $today" >>/spare/local/files/eod_complete.txt

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
echo "BarEnd $today" >>/spare/local/files/eod_complete.txt
