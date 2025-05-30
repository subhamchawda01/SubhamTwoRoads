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
mkdir -p /NAS1/data/ORSData/ORSBCAST_MULTISHM/$yyyy/$mm/$dd


if [ $MODE == "INTRADAY" ]; then
  echo "INTRADAY"
  cd /NAS1/data/ORSData/ORSBCAST_MULTISHM/$yyyy/$mm/$dd
  rm -rf `ls  | grep -v ".gz"`
  rsync -avz --progress 10.23.227.63:/spare/local/ORSBCAST_MULTISHM/NSE/*${today}* /NAS1/data/ORSData/ORSBCAST_MULTISHM/$yyyy/$mm/$dd
  /home/dvcinfra/latency_reports/generate_latency_reports_multishm.sh $today ;
  /home/dvcinfra/latency_reports/generate_exchange_latency_reports.sh $today $MODE;
elif [ $MODE == "NORMAL" ]; then
  echo "NORMAL"
  cd /NAS1/data/ORSData/ORSBCAST_MULTISHM/$yyyy/$mm/$dd
  rm -rf `ls  | grep -v ".gz"`
  rsync -ravz --progress 10.23.227.63:/spare/local/ORSBCAST_MULTISHM/NSE/*${today}*  /NAS1/data/ORSData/ORSBCAST_MULTISHM/$yyyy/$mm/$dd
  /home/dvcinfra/latency_reports/generate_latency_reports_multishm.sh $today ;
  #assuming all files are present under /NAS1/data/ORSData/NSE/${yyy}/${mm}/${dd}
  /home/dvcinfra/latency_reports/generate_exchange_latency_reports.sh $today $MODE;
else
  echo "MODE - $MODE Not Supported"
  exit
fi

