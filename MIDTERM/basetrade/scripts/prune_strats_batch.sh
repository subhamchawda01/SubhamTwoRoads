#!/bin/bash

# CONFIGFILE="/home/dvctrader/modelling/prune_strats_config/config.hrishav_staged.txt";
USAGE="$0 CONFIGFILE NORMAL/STAGED";
if [ $# -lt 1 ] ; then echo $USAGE; exit; fi

CONFIGFILE=$1;
SorN=$2;
if ! [[ $SorN == "NORMAL" || $SorN == "STAGED" ]]; then echo $USAGE; exit; fi

PRUNE_PERF_SCRIPT="$HOME/basetrade/scripts/prune_strats_for_day_from_periods.pl";
PRUNE_SIMILAR_SCRIPT="$HOME/basetrade/scripts/prune_similar_strats.pl";

tail -n +2 $CONFIGFILE | awk '{print $1,$2}' | while read shctp; do
  read SHC TP <<< $shctp;
  echo $SHC ";" $TP ";" $shctp; 
  $PRUNE_PERF_SCRIPT $SHC $TP $CONFIGFILE 1 $SorN;
  $PRUNE_SIMILAR_SCRIPT $SHC $TP TODAY-1 200 0.7 kCNAPnlSharpeAverage 1 $SorN;
done
