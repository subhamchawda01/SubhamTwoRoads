#!/bin/bash

if [ $# -lt 2 ]; then echo "$0 date_file stratfile [Y plot]"; fi

dates=`cat $1`
strat_fl=$2
cnt=0;
id=`whoami | sum | awk '{print $1}'`0
t=$id
for dt in $dates
do
    while [ $cnt -gt 5 ] ;do
	sleep 2;
	cnt=`ps -ef|grep sim_strategy|grep $USER|wc -l`
    done
    ~/basetrade/scripts/run_sim_with_date.sh $dt $strat_fl ${t} 2>/dev/null &
    let t=$t+1
    let cnt=$cnt+1
done
while [ $cnt -gt 0 ] ;do
    sleep 1;
    cnt=`ps aux | grep sim_strategy | grep $strat_fl | wc -l`
done
if [ "$3" == "Y" ];
then
    t=$id
    for dt in $dates
    do
	~/basetrade/scripts/plot_trades_pnl_utc.pl /spare/local/logs/tradelogs/trades.$dt.${t}
	sleep 1; let t=$t+1
    done
fi