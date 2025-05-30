#!/bin/bash
source $HOME/.bash_aliases

USAGE="$0 date [continue_poll(0/1)=0]";
if [ $# -lt 1 ] ;
then
  echo $USAGE;
  exit;
fi

# Config-file example
# 534830 fr216
# 534832 fr215
# 534836 fr216
# 534840 chi13

function printpnl() {
  pnl_str=$( 
    while read -u10 line; do
      qid=`echo $line | awk '{print $1}'`;
      srv_str=`echo $line | awk '{print $2}'`;
      srv=`alias ssh"$srv_str" | awk '{print $3}'`;
      printf "%-15s" "$qid"
    
      if [[ $date == $curr_date ]]; then
        logfl="/spare/local/logs/tradelogs/log.$date.$qid"
        ssh dvctrader@$srv "if [ -f $logfl ]; then grep OpenPnl: $logfl | tail -n1 | tr -d '\n'; fi"
      else
        yyyy=${date:0:4}; mm=${date:4:2}; dd=${date:6:2};
        logfl="/NAS1/logs/QueryLogs/$yyyy/$mm/$dd/log.$date.$qid.gz";
        if [ -f $logfl ]; then zgrep OpenPnl: $logfl | tail -n1 | tr -d '\n'; fi
      fi
      echo
    done 10< $cfg
  )
  if [ $continue -eq 1 ]; then clear; fi
  echo "$pnl_str"
}

curr_date=`date +%Y%m%d`
cfg="$HOME/dvccode/configs/mrt_query_server_map"
date=$1
if [ -z $date ]; then date=`date +%Y%m%d`; fi
continue=${2:-0}

if [ $continue -ne 1 ]; then
  printpnl
else
  clear
  while : ; do
    printpnl
    printf "%15s" "---------" 
    sleep 5
  done
fi
