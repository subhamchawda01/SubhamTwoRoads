#!/bin/bash

if [ $# -lt 6 ]; then
  echo "$0 <fld_path> <event_id> <event_time> <query_runtime_minutes> <event_datfile> <shclist> [<paramfile_sample>] ";
  exit 1;
fi

REPO=$HOME"/basetrade";
AfScriptsDir=$REPO"/AflashScripts";

fld_path=$1;
ev_id=$2;
ev_time=$3;
run_mins=$4;
ev_datfile=$5;
shclist=$6;
paramsample=$AfScriptsDir"/paramfile_sample";
if [ $# -gt 6 ]; then paramsample=$7 ; fi

if [ ! -f $shclist ]; then
  echo "File $shclist NOT found";
  exit 1;
fi

for shc in `awk '{print $1}' $shclist` ; do mkdir -p $fld_path"/"$shc ; done;

time_prefix=`echo $ev_time | egrep -o '[a-zA-Z]*_'`;
time_numeric=`echo $ev_time | sed 's/[a-zA-Z]*_//g'`;
time_minutes=$(( ((time_numeric/100) * 60) + time_numeric%100 ));
time_min_bef=$(( time_minutes - 15 ));
time_min_aft=$(( time_minutes + run_mins ));
time_bef=$time_prefix""$(( ((time_min_bef/60) * 100) + time_min_bef%60 ));
time_aft=$time_prefix""$(( ((time_min_aft/60) * 100) + time_min_aft%60 ));

cat $shclist | while read shcline ; do 
  shcwords=( $shcline );
  shc=${shcwords[0]};
  $findpxch_script $shc $ev_datfile $ev_time $time_bef $time_aft $fld_path 2 5 10 20 1>/dev/null 2>&1;
done;

# After this check the beta and add it in /spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt

cat $shclist | while read shcline ; do 
  shcwords=( $shcline );
  if [[ ${#shcwords[@]} -lt 4 ]]; then
    echo "Insufficient tokens, line: "$shcline;
    continue;
  fi;
  shc=${shcwords[0]};
  uts=${shcwords[1]};
  mur=${shcwords[2]};
  maxuts=${shcwords[3]};
  minpxch=0;
  if [[ ${#shcwords[@]} -gt 4 ]]; then minpxch=${shcwords[4]}; fi;
  shcpath=$fld_path"/"$shc;
  mkdir -p $shcpath;

  paramfile=$shcpath/param_"$shc"_af_agg;
  cat $paramsample | sed "s,UNIT_TRADE_SIZE [0-9]*,UNIT_TRADE_SIZE $uts,g" | sed "s,MAX_UNIT_RATIO [0-9]*,MAX_UNIT_RATIO $mur,g" | sed "s,AF_EVENT_ID .*,AF_EVENT_ID $ev_id,g" | sed "s,AF_EVENT_MAX_UTS_PXCH [0-9]*,AF_EVENT_MAX_UTS_PXCH $maxuts,g" | sed "s,AGGRESSIVE [.0-9]*,AGGRESSIVE $minpxch,g" > $paramfile;

#  echo "PARAMVALUE AF_EVENT_MAX_UTS_PXCH $maxuts" >> $paramfile;

  modelfile=$shcpath/model_"$shc"_empty;
  echo -e "MODELINIT DEPBASE $shc Midprice Midprice\nMODELMATH LINEAR CHANGE\nINDICATORSTART\nINDICATOREND\n" > $modelfile;

  sttfile=$shcpath"/w_af_exec";
  echo -e "STRATEGYLINE $shc EventBiasAggressiveTrading $modelfile $paramfile $time_bef $time_aft 1111" > $sttfile; 

  sttlistfile=$shcpath"/stratlist_agg";
  echo $sttfile > $sttlistfile;
done;

runsim_script=$AfScriptsDir"/run_simulations_to_result_dir.pl";
datelist=$fld_path"/dates_2yrs";
if [[ ! -f $datelist ]]; then
    tail -n +2 $ev_datfile | awk -F, '{print $1}' > $datelist;
fi;

results_fld=$fld_path"/results_aggress";

cat $shclist | while read shcline ; do 
  shcwords=( $shcline );
  shc=${shcwords[0]};
  shcpath=$fld_path"/"$shc;
  sttlistfile=$shcpath"/stratlist_agg";
  echo "$runsim_script $shc $sttlistfile $datelist ALL $results_fld";
done;

