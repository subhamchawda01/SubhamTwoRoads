#!/bin/bash
mail_file="/tmp/file_update_logs_mail.txt"
log_file="/tmp/file_update_logs_mail_speed_test.txt"
status_file="/tmp/file_to_mail_already_sent_error"
DEST_SERVER="10.23.227.63"

>$mail_file
>$log_file

check_internet_speed(){
  echo "Checking Internet Speed..."
  curl -s https://raw.githubusercontent.com/sivel/speedtest-cli/master/speedtest.py | python - >$log_file 
  download_speed=`grep 'Download:' $log_file | awk '{print $2}' | cut -d'.' -f1` 
  download_speed_sz=`grep 'Download:' $log_file | awk '{print $3}'`
  upload_speed=`grep 'Upload:' $log_file | awk '{print $2}' | cut -d'.' -f1` 
  upload_speed_sz=`grep 'Upload:' $log_file | awk '{print $3}'`
  echo "Download Speed: $download_speed $download_speed_sz Upload Speed: $upload_speed $upload_speed_sz "
  if [[ $download_speed_sz != 'Mbit/s' ]] || [[ $upload_speed_sz != 'Mbit/s' ]] || [[ $download_speed -lt 15 ]] || [[ $upload_speed -lt 15 ]] ;then
    echo "ERROR: Slow Speed Internet On Local servers Download $download_speed $download_speed_sz  Upload: $upload_speed $upload_speed_sz "
    echo "ERROR: Slow Speed Internet On Local servers Download $download_speed $download_speed_sz  Upload: $upload_speed $upload_speed_sz " >>$mail_file
  fi  
}

check_ping_drops(){
  echo "Checking Ping and Drops..."
  ping -c50 -w 100 $DEST_SERVER >$log_file 2>&1
  drops_count=`grep 'packets transmitted' $log_file | awk '{print $6}' | cut -d'%' -f1`
  time_taken=`grep 'packets transmitted' $log_file | awk '{print $10}' | cut -d'm' -f1`
  echo "Drops: $drops_count time taken: $time_taken"
  if [[ $drops_count -gt 1 ]] || [[ $time_taken -gt 80000 ]]; then
      echo "ERROR: We have Drops: $drops_count or Time_taken is greater than 9k: $time_taken" 
      echo "ERROR: We have Drops: $drops_count or Time_taken is greater than 9k: $time_taken" >>$mail_file
  fi

}

check_download_speed(){
   echo "Checking Download Speed..."
   rsync -avz --progress 10.23.227.65:/home/dvcinfra/important/NSE_BANKNIFTY_FUT_20211125_20211101.gz /home/dvcinfra/trash/ >$log_file 2>&1 & 
   sync_id=$!
   echo "Process id: $sync_id"
   sleep 20
   kill $sync_id
   sleep 3
   current_speed_=`tail $log_file | tail -n3 | head -n1 | awk '{print $29}'`
   echo "Speed to IND is $current_speed_"
   if [[ $current_speed_ != *MB* ]] ; then 
      echo "ERROR: Low Speed to the Ind servers: $current_speed_"
      echo "ERROR: Low Speed to the Ind servers: $current_speed_" >>$mail_file
   fi
}

check_upload_speed(){
# empty for now as we have always good upload 
  echo "Checking Upload Speed..."
  echo "";
}

YYYYMMDD=`date +%Y%m%d`
prev_working_day=$YYYYMMDD
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_working_day T`
while [ $is_holiday = "1" ]
do
     prev_working_day=`/home/pengine/prod/live_execs/update_date $prev_working_day P A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_working_day T`
done
echo "Previous Working Day: $prev_working_day"
hhmmss=$(date "+%H%M%S")
if [ "$hhmmss" -gt "030000" ] && [ "$hhmmss" -lt "101000" ]; then
    echo "Production Timmings: $hhmmss"
    exit;
fi

check_internet_speed
check_ping_drops
check_download_speed
check_upload_speed


if [[ -s $mail_file ]] && [[ ! -f $status_file ]]; then
  echo "Sending Speed alert mail"
  cat $mail_file | mailx -s "Slow Internet Speed" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
  touch $status_file
  exit
fi
if [[ ! -s $mail_file ]] && [[ -f $status_file ]]; then 
  rm $status_file ; 
  echo "" | mailx -s "Line is Working Fine " -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in;
fi
