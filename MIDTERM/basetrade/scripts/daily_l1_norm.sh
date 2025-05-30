#!/bin/bash

if [ $# -ne 1 ];
then
    echo "$0 DATE";
    exit;
fi

YYYYMMDD=$1;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi


for symbol in `cat /spare/local/tradeinfo/sources.txt` ;
do
  $HOME/basetrade/scripts/compute_daily_l1_norm.pl $symbol $YYYYMMDD 2>/dev/null;
done

$HOME/basetrade/scripts/sync_dir_to_all_machines.pl /spare/local/L1Norms/$YYYYMMDD;
