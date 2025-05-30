#!/bin/bash

diskspace_file="/tmp/external_disk_space_report.txt"
DISK_TO_IGNORE="/home/dvcinfra/important/external_hardrive_to_ignore_for_mail.txt"
declare -A server_to_ip_map
server_to_ip_map=( ["LOCAL26"]="10.23.5.26" \
                   ["LOCAL42"]="10.23.5.42" \
                   ["LOCAL66"]="10.23.5.66" \
                   ["LOCAL67"]="10.23.5.67" )

> $diskspace_file
for server in "${!server_to_ip_map[@]}";do
  echo "Server: $server IP:  ${server_to_ip_map[$server]}"
  echo "Server: $server IP:  ${server_to_ip_map[$server]}" >>$diskspace_file
  ssh ${server_to_ip_map[$server]} "df -h" | awk '{print $5, $6}' | awk '{if($1>90) print $0}' | grep  .[0-9] | grep -vwf $DISK_TO_IGNORE  >>$diskspace_file
done


if [ `cat $diskspace_file | wc -l` -gt 4 ]; then
  cat $diskspace_file | mail -s "--EXTERNALDISKSPACE ALERT --NEED ATTENTION" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in 
fi

