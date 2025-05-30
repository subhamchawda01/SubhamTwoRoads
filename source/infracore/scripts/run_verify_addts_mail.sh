#!/bin/bash

declare -A server_to_ip_map

server_to_ip_map=( ["IND14"]="10.23.227.64" \
                   ["IND15"]="10.23.227.65" \
                   ["IND19"]="10.23.227.69" \
                   ["IND20"]="10.23.227.84" \
                   ["IND16"]="10.23.227.81" \
                   ["IND17"]="10.23.227.82" \
                   ["IND18"]="10.23.227.83" \
                   ["IND23"]="10.23.227.72" \
		   ["IND22"]="10.23.227.71")

today_=`date +"%Y%m%d"`

dt=`date +%Y%m%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $dt T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

>/tmp/verify_addt_mail_file

for server in "${!server_to_ip_map[@]}";
do

  host_name_=`ssh dvcinfra@${server_to_ip_map[$server]} "hostname"`
  rm -rf /tmp/verify_addt_mail_file_${host_name_}_${today_}
  echo "$server"
  ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/verify_addts.sh" &

done
wait

for server in "${!server_to_ip_map[@]}";
do

  host_name_=`ssh dvcinfra@${server_to_ip_map[$server]} "hostname"`
  echo "FOR: $server $host_name_"
  echo $server  ${server_to_ip_map[$server]}
  scp ${server_to_ip_map[$server]}:/tmp/verify_addt_mail_file_${host_name_}_${today_} /tmp/verify_addt_mail_file_${host_name_}_${today_}
  echo "============ $server ================" >>/tmp/verify_addt_mail_file

  if [ `grep -a "ADDTS VERIFICATION DONE" /tmp/verify_addt_mail_file_${host_name_}_${today_} | wc -l ` == "1" ]; then
    ls -lrt /tmp/verify_addt_mail_file_${host_name_}_${today_}
    head -n -1 /tmp/verify_addt_mail_file_${host_name_}_${today_} >>/tmp/verify_addt_mail_file
    echo -e "\n\n" >>/tmp/verify_addt_mail_file
  else
    echo "ADDTS VERIFICATION FAILED"
    echo " => FAILED TO VERIFY ADDTS " >> /tmp/verify_addt_mail_file 
  fi

done

echo -e "\n" >> /tmp/verify_addt_mail_file

cat /tmp/verify_addt_mail_file | mailx -s "ADDTS VERIFICATION [${today_}]" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, nseall@tworoads.co.in
