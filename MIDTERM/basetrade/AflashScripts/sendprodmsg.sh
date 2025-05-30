#!/bin/bash

if [ $# -lt 2 ]; then
  echo "$0 <shortcode_list> <LOAD/PKILL/SEETRADES/SEETRADES_FULL/LISTPROCS/user_msg_in quotes> [<DATE>]";
  exit 1;
fi;

# Each line in the shclist file has the following format: <shc> ..
shclist=$1;
usermsg=$2;
curr_date=`date +'%Y%m%d'`;
if [ $# -gt 2 ]; then date=$3;
else date=$curr_date; fi;
year=${date:0:4}; mm=${date:4:2}; dd=${date:6:2};
echo $date;

if [ $date -lt "20160810" ]; then use_old_mapping=1; fi;

source $HOME/.bash_aliases;

queryids_file=$HOME/basetrade/AflashScripts/queryids_map.txt;
if [ -n "$use_old_mapping" ]; then queryids_file=$HOME/basetrade/AflashScripts/queryids_map_old.txt; fi;

machine_shc_file=/home/pengine/prod/live_configs/machine_shc.txt;
onload_exec=/home/dvctrader/LiveExec/ModelScripts/onload_start_real_trading.sh;
stop_exec=/home/dvctrader/LiveExec/ModelScripts/stop_real_trading.sh;

function getonloadcmd {
  shc=$1;
  sttpath=$2;
  query_id=$3;
  query_out=$4;
  serv_id=$5;
    
  if ssh dvctrader@$serv_id test ! -e $onload_exec; then
    echo " Error: tradexec: $onload_exec does NOT exist in $serv_id $shc..";
    return 1;
  fi;

  local exchange=`$HOME/basetrade_install/bin/get_contract_specs $shc $date EXCHANGE | awk '{print $2}'`;
  if [ -z $exchange ] ; then
    echo "Couldn't find the exchange for shortcode $shc.. skipping this $shc";
    return 1;
  fi;

  cmd="$onload_exec $exchange $query_id $sttpath OFF NF OFF GLOBAL:AFLASH";
  if [[ $exchange == "CME" && ($shc != "NG_0" && $shc != "NG_1") ]] ; then cmd=$cmd" FPGA"; fi;

  cmd=$cmd" >> $query_out 2>&1";
  echo $cmd;
}

function getstopcmd {
  shc=$1;
  query_id=$2;

  local exchange=`$HOME/basetrade_install/bin/get_contract_specs $shc $date EXCHANGE | awk '{print $2}'`;
  if [ -z $exchange ] ; then
    echo "Couldn't find the exchange for shortcode $shc.. skipping this $shc";
    return 1;
  fi;

  cmd="$stop_exec $exchange $query_id 1>/dev/null 2>&1";
  echo $cmd;
}

ev_id=`grep EVENT_ID $shclist | awk '{print $2}' | head -1`;

for shc in `grep SHORTCODE $shclist | awk '{print $2}'`; do
  shc_idx=`grep -w $shc $queryids_file | awk '{print $2}'`;
  serv_id=`grep -w $shc $machine_shc_file | awk '{print $1}'`;

  if [ -z $use_old_mapping ]; then query_id=$(printf '36%03d%03d' "$ev_id" "$shc_idx"); 
  else query_id=$shc_idx; fi;

  if [ -z "$serv_id" ] || [ -z "$query_id" ]; then
    echo " Either $serv_id or $query_id is empty..";
    continue;
  fi;

  if [ $usermsg == "LOAD" ]; then
    remote_dir="/home/dvctrader/af_strats/general/"$shc;
    remote_strat_path=$remote_dir"/w_af_exec_"$query_id;

    if ssh dvctrader@$serv_id test ! -e $remote_strat_path ; then
      echo " Error: strat_file: $remote_strat_path does NOT exist in $serv_id.. Skipping this shortcode";
      continue;
    fi

    if [[ $serv == fr* ]]; then
      checksum=`ssh dvctrader@$serv_id "grep EventBiasAggressiveTrading /spare/local/files/EUREX/eti_md5sum_strategy_algocode_database.db" | awk '{print \$1}'`;
      if [ -z "$checksum" ]; then
        echo "Error: No checksum for EventBiasAggressiveTrading in eti_md5sum_strategy_algocode_database in $serv_id $serv..";
        continue;
      fi;
    fi;

    query_out=$remote_dir/trade_"$date";
    onload_cmd=$(getonloadcmd $shc $remote_strat_path $query_id $query_out $serv_id);

    if [ -z "$onload_cmd" ]; then
      echo "Error: Onload cmd could not be generated for shortcode $shc $serv..";
      continue;
    fi;

    echo $onload_cmd;
    ssh dvctrader@$serv_id "$onload_cmd &";
    echo "$shc LOADED in server $serv_id $serv";

  elif [ $usermsg == "STOP" ] || [ $usermsg == "PKILL" ]; then
    stop_cmd=$(getstopcmd $shc $query_id);

    if [ -z "$stop_cmd" ]; then
      echo "Error: Stop cmd could not be generated for shortcode $shc $serv..";
      continue;
    fi;

    echo $stop_cmd;
    ssh dvctrader@$serv_id "$stop_cmd &";
    echo "$shc Process Stopped in server $serv_id $serv";

  elif [ $usermsg == "LISTPROCS" ]; then
    ssh dvctrader@$serv_id "ps -efH | grep af_tradexec";
  
  elif [ $usermsg == "SEETRADES" ]; then
    echo "Trades for $date.$query_id:";
    if [ "$date" == "$curr_date" ]; then
      ssh dvctrader@$serv_id "tail -n1 /spare/local/logs/tradelogs/trades.$date.$query_id";
    else
      tail -n1 /NAS1/logs/QueryTrades/$year/$mm/$dd/trades."$date"."$query_id";
    fi;
  
  elif [ $usermsg == "SEETRADES_FULL" ]; then
    echo "Trades for $date.$query_id:";
    if [ "$date" == "$curr_date" ]; then
      ssh dvctrader@$serv_id "cat /spare/local/logs/tradelogs/trades.$date.$query_id";
    else
      cat /NAS1/logs/QueryTrades/$year/$mm/$dd/trades."$date"."$query_id";
    fi;

  elif [ $usermsg == "SACI" ]; then
    SACI=`zgrep -m1 SACI /NAS1/logs/QueryLogs/$year/$mm/$dd/log."$date"."$query_id".gz | awk '{print $2}' | sed s/,//g`
    T2T=`/home/pengine/prod/live_execs/ors_binary_reader $shc $date | grep $SACI | grep -m1 Seqd | awk '{print ($10-$12)*1000000;}'`
    EVTIME=`/home/pengine/prod/live_execs/ors_binary_reader $shc $date | grep $SACI | grep -m1 Seqd | awk '{print $12;}'`
    printf "%-16s %s serv: %-16s saci: %-8s t2t: %-12s evtime: %s\n" $shc $query_id $serv_id $SACI $T2T $EVTIME
  
  else
    ssh dvctrader@$serv_id "/usr/sbin/send_user_msg $usermsg --traderid $query_id";
    echo "user_msg passed in server $serv_id $serv";
  fi;
done;


shc=`grep -m1 SHORTCODE $shclist | awk '{print $2}'`
shc_idx=`grep -w $shc $queryids_file | awk '{print $2}'`;
if [ -z $use_old_mapping ]; then query_id=$(printf '36%03d%03d' "$ev_id" "$shc_idx"); 
else query_id=$shc_idx; fi;

if [ $usermsg == "SACI" ]; then
  SACI=`zgrep -m1 SACI /NAS1/logs/QueryLogs/$year/$mm/$dd/log."$date"."$query_id".gz | awk '{print $2}' | sed s/,//g`
  EVTIME=`/home/pengine/prod/live_execs/ors_binary_reader $shc $date | grep $SACI | grep -m1 Seqd | awk '{print int($12);}'`
  echo 
  echo "$HOME/basetrade_install/bin/mkt_trade_logger SIM $shc $date | grep ^$EVTIME | head -10"
  $HOME/basetrade_install/bin/mkt_trade_logger SIM $shc $date | grep ^$EVTIME | head -10
fi

