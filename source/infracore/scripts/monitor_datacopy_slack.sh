#!/bin/bash

slack_exec="/home/pengine/prod//live_execs/send_slack_notification"
slack_channel="datacopy-nse"
ind12=10.23.227.62
ind13=10.23.227.63
ind21=10.23.227.66
worker='dvctrader@54.90.155.232'
warning_mail_file=/tmp/warning_mail_file_file_alrt
>$warning_mail_file
generic_data="/spare/local/MDSlogs/GENERIC/"
generic_nifty_="/spare/local/MDSlogs/GENERIC_NIFTY/"

data_dir="/spare/local/MDSlogs/GENERIC/ALLDATA_${date}/"
converted_dest_dir="/spare/local/MDSlogs/NSE/"
data_to_convert_dir="${data_dir}/DATA_TO_CONVERT/"
declare -A server_to_ip_map
server_to_ip_map=( ["IND12"]="10.23.227.62" \
                   ["IND13"]="10.23.227.63" \
                   ["IND21"]="10.23.227.66" \
                   ["WORKER"]="54.90.155.232" )

send_mail() {
  cat ${warning_mail_file} | mailx -s "$YYYYMMDD $1" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in naman.jain@tworoads-trading.co.in
}

send_slack_alert(){
  $slack_exec $slack_channel DATA $1
}

check_ind_alive(){
        count_=0
        for server in "${!server_to_ip_map[@]}";do
          echo "Server: $server"
          while true;do
            echo "checking: ${server_to_ip_map[$server]}"
            ping -c1 -w 10 ${server_to_ip_map[$server]}
                if [ $? -ne 0 ] ; then
                        count_=$((count_ +1))
                        sleep 5m; echo "Retrying the connection"; continue;
                fi
            if [[ $count_ -gt 6 ]]; then
              count_=0;
              send_slack_alert "SERVER NOT REACHABLE $server"
            fi
            break;
          done
        done
}

check_data_generic(){
  gen13=`ssh $ind13 "ls $generic_data | grep '^NSE_' | wc -l"`
  gennif13=`ssh $ind13 "ls $generic_nifty_ | grep '^NSE_' | wc -l"`
  tot13=$((gen13 + gennif13))
  gen22=`ssh $ind21 "ls $generic_data | grep '^NSE_' | wc -l"`
  gennif22=`ssh $ind21 "ls $generic_nifty_ | grep '^NSE_' | wc -l"`
  tot22=$((gen22 + gennif22))
  total_files=$((tot13 + tot22))
  send_slack_alert "GENERICDATA:\tIND13_GENERIC:$gen13\t\tIND13_GENERIC_NIFTY:$gennif13\t\tTOT13:$tot13\t\tIND21_GENERIC:$gen22\t\tIND21_GENERIC_NIFTY:$gennif22\t\tTOT22:$tot22\t\tTOTAL_FILES:$total_files";
}

check_data_converted(){
  echo "Check_data_converted"
  remote_data_dir="/spare/local/MDSlogs/NSE/"
  ind12_wc=`ssh $ind12 "ls $remote_data_dir |wc -l"`
  ind13_wc=`ssh $ind13 "ls $remote_data_dir |wc -l"`
  ind21_wc=`ssh $ind21 "ls $remote_data_dir |wc -l"`
  send_slack_alert "CONVERTION\t\tSTATUS:"
  send_slack_alert "IND12_NSE:$ind12_wc\t\tIND13_NSE:$ind13_wc\t\tIND21_NSE:$ind21_wc";
  running_12=`ssh $ind12 "ps aux | grep convert_nse_logged_data | grep sh | grep -v grep  | wc -l"`
  running_13=`ssh $ind13 "ps aux | grep convert_nse_logged_data | grep sh | grep -v grep  | wc -l"`
  running_22=`ssh $ind21 "ps aux | grep convert_nse_logged_data | grep sh | grep -v grep  | wc -l"`
  send_slack_alert "IND12_RUNNING:$running_12\t\tIND13_RUNNING:$running_13\t\tIND21_RUNNING:$running_22";
}

check_mtbt_lines_working(){
  echo "Check_mtbt_lines_working"
  channel_info13=`ssh $ind13 "grep STARTED /spare/local/MDSlogs/nseshm_writer_dbg_${YYYYMMDD}.log | wc -l"`
  channel_info22=`ssh $ind21 "grep STARTED /spare/local/MDSlogs/nseshm_writer_dbg_${YYYYMMDD}.log | wc -l"`
  send_slack_alert "Channel\tReceiving\tData\tIND13:\t$channel_info13\t\t\t\t\tIND21:\t$channel_info22" 
}

check_script_already_running(){
echo "Check_script_already_running"
  running_=`ps aux | grep monitor_datacopy_slack.sh | grep -v grep  | wc -l`
  echo "Running status $running_"
  [[ $running_ -gt 2 ]] && { echo "script already Running"; exit;};
}

get_date_check(){
  echo "Get_date_check"
  YYYYMMDD=$(date "+%Y%m%d")
  hhmmss=$(date "+%H%M%S")
  DD=${YYYYMMDD:6:2}
  MM=${YYYYMMDD:4:2}
  YY=${YYYYMMDD:2:2}
  YYYY=${YYYYMMDD:0:4}
  echo "DATE: $YYYYMMDD"
  prev_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A 1`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  if [ $is_holiday = "1" ] ; then
     echo "NSE Holiday. Exiting...sleeping 100m";
    sleep 100m
  fi
  is_cd_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE_CD $YYYYMMDD T`
  echo "CD HOLIDAY: $is_cd_holiday"
  [[ $is_cd_holiday = "1" ]] &&  [[ $cd_prev -ne $YYYYMMDD ]] && { send_slack_alert "CD\tHOLIDAY\t\t$YYYYMMDD";}
  cd_prev=$YYYYMMDD
}

check_sync_data(){
  echo "Check_sync_data"
  remote_data_dir="/spare/local/MDSlogs/${YYYY}/${MM}/${DD}/"
  local_data_dir="/NAS1/data/NSELoggedData/NSE/${YYYY}/$MM/${DD}/"
  local_wc=`ls $local_data_dir |wc -l`
  ind12_wc=`ssh $ind12 "ls $remote_data_dir |wc -l"`
  ind13_wc=`ssh $ind13 "ls $remote_data_dir |wc -l"`
  ind21_wc=`ssh $ind21 "ls $remote_data_dir |wc -l"`
  worker_wc=`ssh $worker "ls $local_data_dir |wc -l"`
  send_slack_alert "DATA\tSYNC\tSTATUS:"
  send_slack_alert "IND12:$ind12_wc\t\tIND13:$ind13_wc\t\tIND21:$ind21_wc\t\tLOCAL:$local_wc\t\tWORKER:$worker_wc";
}

prev_date_check=$(date "+%Y%m%d")
line_check=0
check_script_already_running
while true; do
  get_date_check
  check_ind_alive
  if [ "$hhmmss" -gt "030000" ] && [ $line_check -eq 0 ]; then
     line_check=1
     send_slack_alert "SETTING\tUP\tFOR\t\t $YYYYMMDD";
     check_mtbt_lines_working
  fi
  if [ "$hhmmss" -gt "100000" ] && [ $line_check -eq 1 ]; then
     line_check=2
     send_slack_alert "Checking\tGENERIC\tDATA\t\t $YYYYMMDD";
     check_data_generic
  fi
  if [ $prev_date_check -ne $YYYYMMDD ];then
    prev_date_check=$YYYYMMDD
    line_check=0
  fi
  if [ "$hhmmss" -gt "030000" ] && [ "$hhmmss" -lt "100000" ]; then
      echo "production timmings. sleep 5m"
      sleep 5m; # production timmings
      continue;
  fi
  check_data_converted
  check_sync_data
  send_slack_alert "\n"
  i=0
  while [[ $i -lt 8 ]]; do
    echo "CHECK SLEEP"
    get_date_check
    sleep 5m
    if [ "$hhmmss" -gt "030000" ] && [ "$hhmmss" -lt "040000" ]; then 
      break
    fi
  i=$(( i +1))
  done
done


