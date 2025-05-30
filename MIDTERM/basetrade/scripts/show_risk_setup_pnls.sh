#!/bin/bash
#Script to be run on NY11 (any user)
#Shows the pnl/pos values computed by the risk monitor setup
USAGE="$0 tag file"
if [ $# -le 0 ]; then
  echo $USAGE;
  echo "Using tag=GLOBAL";
fi
tag=GLOBAL
export TZ="Asia/Tokyo"
today=`date +%Y%m%d --date='1 day ago'`
log_file=/spare/local/logs/prod_risk_server_log.$today
if [ $# -eq 1 ]; then
  tag=$1
fi

if [ $# -eq 2 ]; then
  tag=$1
  log_file=$2
fi

grep "Read update" $log_file | grep $tag | tac | awk -F"," '!a[$2]++' | python /home/pengine/prod/live_scripts/parse_risk_setup_pnls.py
