#!/bin/bash

if [ $# -lt 2 ]; then
  echo "$0 <shortcode_list> <datelist/start_date>";
  exit 1;
fi;

# Each line in the prodlist file has the following format: <shc> ..
prodlist=$1;
datelist=$2;

source $HOME/.bash_aliases;
new_queryids_file=$HOME/basetrade/AflashScripts/queryids_map.txt;
old_queryids_file=$HOME/basetrade/AflashScripts/queryids_map_old.txt;
machine_shc_file=/home/pengine/prod/live_configs/machine_shc.txt;

shcvec=`grep "^SHORTCODE " $prodlist | awk '{print $2}'`;
eventid=`grep EVENT_ID $prodlist | awk '{print $2}' | head -1`;

if [ -z "$shcvec" ] || [ -z "$eventid" ] ; then
  echo "Error: Couldn't find event_id and/or shortcodes in the prodlist";
  exit 1;
fi;

if [ -f "$datelist" ]; then
  datevec=`cat $datelist`;
else
  datevec=`grep ^$eventid /spare/local/tradeinfo/Alphaflash/Estimates/estimates_201* | cut -d":" -f1 | cut -d"_" -f2 | awk -vsd=$datelist '{ if($1 >= sd) { print $1; } }'`;
fi;

echo SHORTCODE ${shcvec[@]} | tr ' ' ',';

for shc in ${shcvec[@]} ; do
  old_qid=`grep -w $shc $old_queryids_file | awk '{print $2}'`;
  shc_idx=`grep -w $shc $new_queryids_file | awk '{print $2}'`;
  new_qid=$(printf '36%03d%03d' "$eventid" "$shc_idx");
  if [ -z "$new_qid" ] ; then continue; fi;

  linep="$shc";
  for dt in ${datevec[@]}; do
    if [ "$dt" -ge "20160810" ]; then query_id=$new_qid; 
    else query_id=$old_qid; fi;

    linep="$linep, ";
    if [ -z "$query_id" ]; then continue; fi;

    year=${dt:0:4}; mm=${dt:4:2}; dd=${dt:6:2};
    trade_fl=/NAS1/logs/QueryTrades/$year/$mm/$dd/trades."$dt"."$query_id";
    if [[ -f "$trade_fl" ]]; then
      pnl=`tail -n1 $trade_fl | awk '{print $9}'`;
      linep="$linep$pnl";
    fi;
  done;
  echo $linep;
done;
