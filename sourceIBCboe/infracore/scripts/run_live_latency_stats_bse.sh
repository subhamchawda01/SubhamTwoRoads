#!/bin/bash
[ $# -ne 2 ] && { echo "PROVIDE DATE MODE[INTRADAY/NORMAL] IN ARGUMENT"; exit; }
if [ $1 == "TODAY" ];
then
	today=`date +"%Y%m%d"`
else
	today=$1;
fi
MODE=$2
yyyy=`date -d $today +"%Y"`
mm=`date -d $today +"%m"`
dd=`date -d $today +"%d"`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`

if [ $is_holiday = "1" ]; then
  echo "holiday $today"
  exit -1
fi

#assuming all the files are present on 5.42
datadir_="/NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/$yyyy/$mm/$dd"
mkdir -p $datadir_
chown -R dvcinfra /NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/
chgrp -R infra /NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/


if [ $MODE == "INTRADAY" ]; then
  echo "INTRADAY"
  cd $datadir_
  rm -rf `ls  | grep -v ".gz"`
  rsync -avz --progress 192.168.132.11:/spare/local/ORSBCAST_MULTISHM/BSE/*${today}* $datadir_
  echo "Computing Latency:"
  /home/pengine/prod/live_scripts/generate_latency_reports_multishm_bse.sh $today ;
  /home/pengine/prod/live_scripts/generate_exchange_latency_reports_bse.sh $today $MODE;

elif [ $MODE == "NORMAL" ]; then
  echo "NORMAL"
  cd $datadir_
  rm -rf `ls  | grep -v ".gz"`
  rsync -ravz --progress 192.168.132.11:/spare/local/ORSBCAST_MULTISHM/BSE/*${today}* $datadir_
  /home/pengine/prod/live_scripts/generate_latency_reports_multishm_bse.sh $today ;
  #assuming all files are present under /NAS1/data/ORSData/NSE/${yyy}/${mm}/${dd}
  /home/pengine/prod/live_scripts/generate_exchange_latency_reports_bse.sh $today $MODE;
else
  echo "MODE - $MODE Not Supported"
  exit
fi

