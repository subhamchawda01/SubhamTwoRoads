#!/bin/bash

email_file=/tmp/check_generic_files_folders_have_entry

declare -A server_to_ip_map
server_to_ip_map=( ["IND12"]='10.23.227.62' \
                   ["IND13"]='10.23.227.63' \
                   ["IND21"]='10.23.227.66' )

>$email_file
date_=`date +%Y%m%d`

for server in "${!server_to_ip_map[@]}";
do
  echo "$server"
  count_nif=`ssh ${server_to_ip_map[$server]} "ls /spare/local/MDSlogs/GENERIC_NIFTY/ | grep '^NSE' | grep -v $date_ | wc -l"`
  count_gen=`ssh ${server_to_ip_map[$server]} "ls /spare/local/MDSlogs/GENERIC/ | grep '^NSE'| grep -v $date_ | wc -l"`
  echo "SERVER:$server GENERIC_NIFTY:$count_nif GENERIC:$count_gen"
  if [[ $count_nif -gt 0  ||  $count_gen -gt 0 ]] ; then
      echo "SERVER:$server GENERIC_NIFTY:$count_nif GENERIC:$count_gen"
      echo "SERVER:$server GENERIC_NIFTY:$count_nif GENERIC:$count_gen" >>$email_file
  fi
done



[ -s $email_file ] && cat $email_file | mail -s "ALERT: FILE EXIST ON GENERIC FOLDERS " -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
