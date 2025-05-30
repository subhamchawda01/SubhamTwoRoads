#!/bin/bash

dt=$1
strat_fl=$2
id=$3
fullprint=$4

if [ -z $id ]; then id=25452; fi

if [ -z $fullprint ];
then 
    echo $dt \
    `~/basetrade_install/bin/sim_strategy SIM $strat_fl $id $dt 3 ADD_DBG_CODE -1 | awk '{ print $2,$3,"||" }'; 
    ~/basetrade_install/ModelScripts/get_pnl_stats_2.pl /spare/local/logs/tradelogs/trades.$dt.$id | sort -n | awk '{print $2,$7,"|"}'`
else
    echo $dt \
    `~/basetrade_install/bin/sim_strategy SIM $strat_fl $id $dt 3 ADD_DBG_CODE -1`
fi
    