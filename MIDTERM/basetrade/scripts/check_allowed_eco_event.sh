#!/bin/bash

if [ $# -lt 3 ]; then echo "$0 <shortcode> <event name> <strat to check> Y/N<graph or not[Y]>"; exit; fi

shortcode=$1
event=$2
strats=$3
starti_stop=$4
showgraph=$5

today=`date +%Y%m%d`

cd ~/infracore; git pull; cd -;
cp ~/infracore/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;

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

    eco_event_val=$(grep -w "$event" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201[23]_processed.txt | grep $today | cut -d' ' -f 3-4 ) # | sed 's/\ /\\\ /g')
    if [ -z "$event" ]; then echo "Event required..."; exit; fi
    dates=( $(grep -w "$event" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201[23]_processed.txt | cut -d' ' -f 5 | awk -F'_' '{ if($1 < '$today') print $0}' | sort -n | tail -n8 ))
    for i in ${dates[*]}; do echo ${i:0:8}; done > ~/t_dates

    function run_sim {

	for d in ${dates[*]}
	do
	    x=$(grep "$event" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2012_processed.txt | grep $d)
	    other_events_at_that_time=($(grep $d ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201[23]_processed.txt | grep -v "$event" | cut -d' ' -f2,3 ));
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
	event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
	if [ $event_present > 0 ]
	then
	    sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	fi
        echo $shortcode $event > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
    done

    echo ">>>Running Without Stopping at the economic event"
    echo ">>>============================================="
    run_sim
    done

    if [ -z $start_stop ]
    then     
	    for d in ${dates[*]}
	    do
	    #date_time=$d'_'$event_time
	        event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
	        if [ $event_present > 0 ]
	        then
	            sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	        fi
	        echo $shortcode $event 300 0 > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	    done

	    echo ">>>Running With Stopping 300 secs before and Starting at the economic event"
	    echo ">>>============================================="
	    run_sim

	    for d in ${dates[*]}
	    do
	    #date_time=$d'_'$event_time
	        event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
	        if [ $event_present > 0 ]
	        then
	            sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	        fi
	        echo $shortcode $event 300 -10 > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	    done
	
	    echo ">>>Running With Stopping 300 secs before and Starting 10 secs after the economic event"
	    echo ">>>============================================="
	    run_sim

	    for d in ${dates[*]}
	    do
	    #date_time=$d'_'$event_time
	        event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
	        if [ $event_present > 0 ]
	        then
	            sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	        fi
	        echo $shortcode $event 600 0 > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	    done

	    echo ">>>Running With Stopping 600 secs before and Starting 0 secs after the economic event"
	    echo ">>>============================================="
	    run_sim

	    for d in ${dates[*]}
	    do
	    #date_time=$d'_'$event_time
	        event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
	        if [ $event_present > 0 ]
	        then
	            sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
        	fi
	        echo $shortcode $event 600 -10 > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
	    done

	    echo ">>>Running With Stopping 600 secs before and Starting 10 secs after the economic event"
	    echo ">>>============================================="
	    run_sim

	    cp ~/infracore/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt
	    if [ "$showgraph" = "Y" ]; then sleep 10; fi;

           for d in ${dates[*]}
            do
            #date_time=$d'_'$event_time
                event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
                if [ $event_present > 0 ]
                then
                    sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
                fi
                echo $shortcode $event 300 -300 > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
            done

            echo ">>>Running With Stopping 300 secs before and Starting 300 secs after the economic event"
            echo ">>>============================================="
            run_sim

            cp ~/infracore/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt
            if [ "$showgraph" = "Y" ]; then sleep 10; fi;

           for d in ${dates[*]}
            do
            #date_time=$d'_'$event_time
                event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
                if [ $event_present > 0 ]
                then
                    sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
                fi
                echo $shortcode $event 600 -600 > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
            done

            echo ">>>Running With Stopping 600 secs before and Starting 600 secs after the economic event"
            echo ">>>============================================="
            run_sim

            cp ~/infracore/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt
            if [ "$showgraph" = "Y" ]; then sleep 10; fi;


   else

           for d in ${dates[*]}
            do
            #date_time=$d'_'$event_time
                event_present=`grep -c "$shortcode_*$event_" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt`;
                if [ $event_present > 0 ]
                then
                    sed -i -e "/$shortcode $event/d" ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
                fi
                echo $shortcode $event $start_stop > ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt;
            done

            echo ">>>Running With given start stop times for the economic event"
            echo ">>>============================================="
            run_sim
            cp ~/infracore/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt ~/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt
   fi
