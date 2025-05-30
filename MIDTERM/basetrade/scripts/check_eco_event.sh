#!/bin/bash

if [ $# -lt 2 ]; then echo "$0 <event name> <strat to check> Y/N<graph or not[Y]>"; exit; fi
event=$1
strats=$2
showgraph=$3

today=`date +%Y%m%d`

cd ~/infracore; git pull; cd -;
cp ~/infracore/SysInfo/BloombergEcoReports/merged_eco_201[234]_processed.txt ~/infracore_install/SysInfo/BloombergEcoReports/

for strat in $strats
do
    echo ; echo "----------------------------------------------------------"; 
    echo "DDDD" $strat "CCCC"

    if [ ! -e $strat ]
    then
	strat=`basename $strat`
	strat=`find ~/modelling/ -name $strat`
    fi
    if [ ! -e $strat ]; then echo "Sorry the strat file $2 not found."; exit; fi;

    eco_event_val=$(grep -w "$event" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201[234]_processed.txt | grep $today | cut -d' ' -f 3-4 ) # | sed 's/\ /\\\ /g')
    if [ -z "$event" ]; then echo "Event required..."; exit; fi
    dates=( $(grep -w "$event" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201[234]_processed.txt | cut -d' ' -f 5 | awk -F'_' '{ if($1 < '$today') print $0}' | sort -n | tail -n5 ))
    for i in ${dates[*]}; do echo ${i:0:8}; done > ~/t_dates

    function run_sim {

	for d in ${dates[*]}
	do
	    x=$(grep "$event" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2012_processed.txt | grep $d)
	    other_events_at_that_time=($(grep $d ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201[234]_processed.txt | grep -v "$event" | cut -d' ' -f2,3 ));
	    echo $x ${other_events_at_that_time[*]}
	done
	~/basetrade/scripts/run_sim.sh ~/t_dates $strat $showgraph | sort -n -k1
#    ~/basetrade/scripts/plot_trades_pnl_utc.pl /spare/local/logs/tradelogs/trades.20121204.20140130
    }

    echo ">>>Running with stopping at the economic event"
    echo ">>>=========================================== "
    run_sim
# take_backup
# for year in 2013 2013;
# do
#     cp ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_${year}_processed.txt ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_${year}_processed.txt_bak
# done

    for d in ${dates[*]}
    do
    #date_time=$d'_'$event_time
	sed -i -e 's/[0-9] '$d'/0 '$d'/' ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201[234]_processed.txt
    done

    echo ">>>Running Without Stopping at the economic event"
    echo ">>>============================================="
    run_sim

    cp ~/infracore/SysInfo/BloombergEcoReports/merged_eco_201[234]_processed.txt ~/infracore_install/SysInfo/BloombergEcoReports/
    if [ "$showgraph" = "Y" ]; then sleep 10; fi;
done
