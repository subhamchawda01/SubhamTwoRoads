#!/bin/bash

today_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
     echo "NSE holiday..., exiting";
     exit
fi

declare -A server_to_ip_map
logs_given="/tmp/logs_for_strat_start_given"
logs_not_given="/tmp/logs_for_strat_start_not_given"
echo "START GIVEN" >$logs_given
echo "START NOT GIVEN" >$logs_not_given

server_to_ip_map=( 
 		["IND14"]="10.23.227.64" \
		["IND15"]="10.23.227.65" \
		["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
		["IND19"]="10.23.227.69" \
		["IND20"]="10.23.227.84" \
		["IND22"]="10.23.227.71" \
                ["IND23"]="10.23.227.72" )

today=`date +"%Y%m%d"`;
for server in "${!server_to_ip_map[@]}";
do
	echo $server 
	echo -n "$server: " >> $logs_given
	echo -n "$server: " >> $logs_not_given
	for strat in `ssh ${server_to_ip_map[$server]} "ls /spare/local/logs/tradelogs/log.$today.*"`; do 
		echo $strat
		line_no=`ssh ${server_to_ip_map[$server]} "grep -nar 'N5HFSAT5Utils31ClientLoggingSegmentInitializerE:Initialize' $strat" | tail -1 | cut -d':' -f1`
  		count_=`ssh ${server_to_ip_map[$server]} "tail -n +$line_no $strat | grep 'Start Trading' | wc -l"`
		strat_id=`echo $strat | awk -F"." '{print $3}'| awk -F":" '{print $1}' | xargs  | tr ' ' '|'`
		echo "$strat_id $count_"
		if [[ $count_ -gt 0 ]]; then
			echo -n "$strat_id " >> $logs_given
		else 
			echo -n "$strat_id " >> $logs_not_given
		fi
	done
	echo "" >> $logs_given
	echo "" >> $logs_not_given
done

echo "" >> $logs_not_given
cat $logs_given >> $logs_not_given
cat $logs_not_given | mailx -s "Strats Start logs" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in 
