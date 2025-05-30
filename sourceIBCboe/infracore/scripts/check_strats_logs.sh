#!/bin/bash

tradelogs="/spare/local/logs/tradelogs/"
querylogs="/spare/local/logs/tradelogs/queryoutput/"
APPEND_FILE="/tmp/log_after_append_strat_logs"
mail_report="/tmp/strat_mail_daily_file.txt"
trade_engine="/tmp/trade_engine_live_mail"
logs_given="/tmp/logs_for_strat_start_given"
>$mail_report
declare -A server_to_ip_map
server_to_ip_map=( ["IND14"]="10.23.227.64" \
                   ["IND15"]="10.23.227.65" \
		["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
                ["IND20"]="10.23.227.84" \
                ["IND22"]="10.23.227.71" \
                ["IND23"]="10.23.227.72" )


today_=`date +"%Y%m%d"`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi


for server in "${!server_to_ip_map[@]}";
do
    channel_check_file="/tmp/strat_check_mail_file_${server}_${today_}"
    rm /tmp/logs_for_strat_start_given

    echo "Server:$server  IP:${server_to_ip_map[$server]}"
    >$trade_engine

        #Check if trade running or not running
	echo "Checking running status for $server"
	ssh dvctrader@${server_to_ip_map[$server]} "crontab -l | grep -v '^#' | grep run | awk '{if(NF == 6) print}'">/tmp/crontab_files
	files=`cat /tmp/crontab_files | awk '{print $6}'`
	for file in $files;
    	do
		echo "RunFile: $file"
		ssh dvctrader@${server_to_ip_map[$server]} "cat $file | grep -v '^#' | grep '/ \|CONFIG'| awk '{print \$6}' | sort | uniq"> /tmp/start_num
		for strat in `cat /tmp/start_num`;
      		do
			if [ `ssh dvctrader@${server_to_ip_map[$server]} "ps aux | grep trade_engine | grep $strat | grep -v grep | wc -l"` -eq 0 ]; then
				echo "$strat" >> $trade_engine
				echo "$strat IS NOT RUNNING ";				
			fi
		done
	done

	#Checking wheter start given to the STRAT or not
	>$logs_given
	for strat in `ssh dvcinfra@${server_to_ip_map[$server]} "ls /spare/local/logs/tradelogs/log.$today_.*"`; do
                echo $strat
                line_no=`ssh dvcinfra@${server_to_ip_map[$server]} "grep -nar 'N5HFSAT5Utils31ClientLoggingSegmentInitializerE:Initialize' $strat" | tail -1 | cut -d':' -f1`
                count_=`ssh dvcinfra@${server_to_ip_map[$server]} "tail -n +$line_no $strat | grep 'Start Trading' | wc -l"`
                strat_id=`echo $strat | awk -F"." '{print $3}'| awk -F":" '{print $1}' | xargs  | tr ' ' '|'`
                echo "$strat_id $count_"
                if [[ $count_ -gt 0 ]]; then
                        echo -n "$strat_id " >> $logs_given
		else
			echo "$strat_id START NOT GIVEN"
                fi
	done

	ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/check_strats_servers_logs.sh"
	rm $channel_check_file
        scp ${server_to_ip_map[$server]}:/tmp/strat_check_mail_file_${today_} $channel_check_file

        echo '<div style="display:flex;justify-content:space-around">' >>$mail_report
        echo "<div style='margin:0 30px;'>" >>$mail_report
        echo "<h3>${server}</h3>">>$mail_report
        echo "<table border='1' id='myTable' class='table table-striped' style='border-collapse: collapse'<thead><tr><th>STRAT_ID</th><th>FUT_0</th><th>FUT_1</th><th>FUT_2</th><th>CHANNELS_ADDED</th><th>GETTING_DATA</th><th>NOT_RUNNING</th><th>START_GIVEN</th></tr></thead><tbody>" >> $mail_report
        
	count=0
 	
	cat $channel_check_file | uniq > /tmp/strat_check_mail_file_${server}_${today_}_tmp        

	#Sorting according to the basis of not running

	count=`wc -l < $channel_check_file`
	sorted_file=/tmp/sorted_strat_check_mail_file_${server}_${today_}
        >$sorted_file	
	last_line=`tail -1 $channel_check_file`
	while IFS= read -r line
        do
                count=$(($count-1))
		
                start_id=$(echo $line | awk '{print $1}' | awk -F ":" '{print $2}' )
                fut_0=$(echo $line | awk '{print $2}' | awk -F ":" '{print $2}' )
                fut_1=$(echo $line | awk '{print $3}' | awk -F ":" '{print $2}' )
                fut_2=$(echo $line | awk '{print $4}' | awk -F ":" '{print $2}' )
                channel=$(echo $line | awk '{print $5}' | awk -F ":" '{print $2}' )
                get_data=$(echo $line | awk '{print $6}' | awk -F ":" '{print $2}' )

                #Check if running of not running
                trade_running=0
                if grep -aq "$start_id" $trade_engine; then
                        trade_running=1
                fi

		start_given="FALSE"
                if grep -aq "$start_id" $logs_given; then
                        start_given="TRUE"
                fi
	        echo "$start_id $fut_0 $fut_1 $fut_2 $channel $get_data $trade_running $start_given"	
		echo "$start_id $fut_0 $fut_1 $fut_2 $channel $get_data $trade_running $start_given" >> $sorted_file
		
		test $count -eq 1 && break;
		
        done < "$channel_check_file"

	cat $sorted_file | sort -rn -k 7 > $channel_check_file
	count=`wc -l < $channel_check_file`	
	
        while IFS= read -r line
        do
                count=$(($count-1))
	
		start_id=$(echo $line | awk '{print $1}')  
		fut_0=$(echo $line | awk '{print $2}') 
		fut_1=$(echo $line | awk '{print $3}')
                fut_2=$(echo $line | awk '{print $4}')
                channel=$(echo $line | awk '{print $5}')
                get_data=$(echo $line | awk '{print $6}' )

		str_trade_not_running="FALSE";
		trade_running=$(echo $line | awk '{print $7}')
		
		test $trade_running -eq 1 && str_trade_not_running="TRUE"  
		
		start_given=$(echo $line | awk '{print $8}')

                #echo "$start_id $fut_0 $fut_1 $fut_2 $channel $get_data $ str_trade_not_running $start_given"
		
		if [[ "$str_trade_not_running" == "TRUE" ]]
		then
			echo -e "<tr bgcolor= "yellow" ><td>$start_id</td><td>$fut_0</td><td>$fut_1</td><td>$fut_2</td><td>$channel</td><td>$get_data</td><td>$str_trade_not_running</td><td>$start_given</td></tr>" >> $mail_report
		else
			echo -e "<tr><td>$start_id</td><td>$fut_0</td><td>$fut_1</td><td>$fut_2</td><td>$channel</td><td>$get_data</td><td>$str_trade_not_running</td><td>$start_given</td></tr>" >> $mail_report
		fi

        done < $channel_check_file
 
    line=$last_line
    echo "</tbody></table>" >> $mail_report
    echo "<div>${line}</div>" >> $mail_report
    echo "</div></body></html>" >> $mail_report

done

(
#   echo "To: raghunandan.sharma@tworoads-trading.co.in"
  echo "To: raghunandan.sharma@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, tarun.joshi@tworoads-trading.co.in, naman.jain@tworoads-trading.co.in"
  echo "Content-Type: text/html; "
  echo Subject: STRAT LOGS VERIFICATION
  echo
  cat /tmp/strat_mail_daily_file.txt
) | /usr/sbin/sendmail -t

