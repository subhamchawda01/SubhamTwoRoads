#!/bin/bash

tradelogs="/spare/local/logs/tradelogs/"
querylogs="/spare/local/logs/tradelogs/queryoutput/"
APPEND_FILE="/tmp/log_after_append_strat_logs"
strat_mail_file="/tmp/strat_mail_daily_file.txt"
>$strat_mail_file
declare -A server_to_ip_map
server_to_ip_map=( ["IND14"]="10.23.227.64" \
                   ["IND15"]="10.23.227.65" \
                ["IND19"]="10.23.227.69"
                ["IND20"]="10.23.227.84" )

today_=`date +"%Y%m%d"`
for server in "${!server_to_ip_map[@]}";
do
    rm -rf /tmp/strat_check_mail_file_${server}_${today_}
    echo $server  ${server_to_ip_map[$server]}
    ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/check_strats_fo_servers_logs.sh"
    if [ $? -ne 0 ];
    then
        echo "${server} => Failed To check Strat logs " >> $strat_mail_file
    else
        scp ${server_to_ip_map[$server]}:/tmp/strat_check_mail_file_${today_} /tmp/strat_check_mail_file_${server}_${today_}
    fi
    ls -lrt /tmp/strat_check_mail_file_${server}_${today_}
done

echo -e "\n" >> $strat_mail_file

for server in "${!server_to_ip_map[@]}";
do
    echo "============ $server ================" >> $strat_mail_file
    cat /tmp/strat_check_mail_file_${server}_${today_} >> $strat_mail_file
    echo -e "\n\n" >> $strat_mail_file
done


cat $strat_mail_file | mailx -s "STRAT LOGS VERIFICATION FO" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in


