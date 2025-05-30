#!/bin/bash

channel_file=/tmp/channel_working_info_nse
email_file=/tmp/email_channal_working_Details

declare -A server_to_ip_map
server_to_ip_map=( ["IND11"]='10.23.227.61' \
                   ["IND12"]='10.23.227.62' \
                   ["IND13"]='10.23.227.63' \
                   ["IND14"]='10.23.227.64' \
                   ["IND15"]='10.23.227.65' \
                   ["IND16"]='10.23.227.81' \
                   ["IND17"]='10.23.227.82' \
                   ["IND18"]='10.23.227.83' \
                   ["IND19"]='10.23.227.69' \
                   ["IND20"]='10.23.227.84' )

ssh 10.23.227.61 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh FUT p4p2 >/dev/null 2>&1" &
ssh 10.23.227.62 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh FUT eth1 >/dev/null 2>&1" &
ssh 10.23.227.63 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh BOTH eth3 >/dev/null 2>&1" &
ssh 10.23.227.64 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh BOTH eth1 >/dev/null 2>&1" &
ssh 10.23.227.65 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh FUT enp101s0f1 >/dev/null 2>&1" &
ssh 10.23.227.81 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh BOTH enp1s0f1 >/dev/null 2>&1" &
ssh 10.23.227.82 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh CASH enp101s0f1 >/dev/null 2>&1" &
ssh 10.23.227.83 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh BOTH enp101s0f1 >/dev/null 2>&1" &
ssh 10.23.227.69 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh FUT enp101s0f1 >/dev/null 2>&1" &
ssh 10.23.227.84 "/home/dvcinfra/important/NseLineCheck/CheckNseLine.sh FUT enp1s0f1 >/dev/null 2>&1" &

sleep 40s
>$email_file

for server in "${!server_to_ip_map[@]}";
do
  echo "$server"
  rm $channel_file
  scp ${server_to_ip_map[$server]}:$channel_file /tmp/
  if [[ -f $channel_file ]] ;then
    cat $channel_file >>$email_file
  else
    echo "server $server File Not Found" >> $email_file
  fi
done


[ -s $email_file ] && cat $email_file | mail -s "--Channel Details-- For IND" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" infra_alerts@tworoads-trading.co.in
#cat $email_file | mail -s "--Channel Details-- For IND" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
