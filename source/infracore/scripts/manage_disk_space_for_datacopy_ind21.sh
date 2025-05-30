#!/bin/bash
mail_alert_file="/tmp/data_copy_space_alert_mails.txt"
>$mail_alert_file
master_server='10.23.227.63'
data_copy_info_file="/spare/local/data_copy_update.txt"

GetPreviousWorkingDay() {
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
    while [ $is_holiday_ = "1" ];do
      YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
      is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
    done
}

fail_alert_and_exit(){
  cat $mail_alert_file | mailx -s "Problem with $HOSTNAME: Not deleteing Data" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
  exit
}

YYYYMMDD=`date +%Y%m%d`;
TODAY_=$YYYYMMDD
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
GetPreviousWorkingDay ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}
echo "Previous Day: $YYYYMMDD"

status_count=`grep $YYYYMMDD $data_copy_info_file  |wc -l`
echo "Status Count $status_count"
if [[ $status_count -lt 4 ]]; then 
  echo "Status Count Less $status_count" >> $mail_alert_file
  echo "Status Count Less $status_count"
  fail_alert_and_exit
fi

run_status=`ps aux | grep convert_nse_logged_data_IND24.sh | grep $YYYYMMDD | grep -v grep | wc -l`
echo "DataCopy Run: $run_status"
if [[ $run_status -gt 0 ]]; then 
  echo "Yesterday Datacopy running $run_status" >> $mail_alert_file
  echo "Yesterday Datacopy running $run_status"
  fail_alert_and_exit
fi

run_status=`ps aux | grep rsync | grep $YYYY | grep MDSlogs | grep -v grep | wc -l`
echo "Sync Run: $run_status"
if [[ $run_status -gt 0 ]]; then 
  echo "Sync Running For Yesterday Datacopy running $run_status" >> $mail_alert_file
  echo "Sync Running For Yesterday Datacopy running $run_status"
  fail_alert_and_exit
fi

dir_tocheck="/spare/local/MDSlogs/${YYYY}/${MM}/${DD}"
echo "Yesterday Directory $dir_tocheck"
master_dir="/spare/local/MDSlogs/${YYYY}/${MM}/${DD}"

mkdir -p $dir_tocheck

echo "Syncing Data To Master $master_server"
echo "rsync -avz ${dir_tocheck}/  $master_server:$dir_tocheck"

rsync -avz ${dir_tocheck}/  $master_server:$dir_tocheck

if [[ $? -ne 0 ]]; then
    echo "Rsync Problem"
    echo "Rsync Problem" >> $mail_alert_file
    fail_alert_and_exit
fi


cd $dir_tocheck
pwd
rm NSE_*
echo "" | mailx -s "Data Removed From MDSlogs for date $YYYYMMDD" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in 

