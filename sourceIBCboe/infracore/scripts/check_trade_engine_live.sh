#!/bin/bash

date=`date +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

true>/tmp/trade_engine_live_mail

#                ["IND12"]="10.23.227.62" \
#                ["IND13"]="10.23.227.63" \
declare -A IND_Server_ip
  IND_Server_ip=( ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
                ["IND20"]="10.23.227.84")


for server in "${!IND_Server_ip[@]}";
do
  echo "for ---- $server";
  #if [ "$server" = "IND11" ] || [ "$server" != "IND12" ] || [ "$server" != "IND13" ]; then
    #echo "entered"
    echo "$server:: ">>/tmp/trade_engine_live_mail
    ssh dvctrader@${IND_Server_ip[$server]} "crontab -l | grep -v '^#' | grep run | awk '{if(NF == 6) print}'">/tmp/crontab_files
    #ssh ${IND_Server_ip[$server]} "crontab -l | grep -v '^#' | grep run">/tmp/crontab_files #backup line
    files=`cat /tmp/crontab_files | awk '{print $6}'`
    #echo -e "for $server\n"

    for file in $files;
    do
      echo -e " $file->\n"

      ssh dvctrader@${IND_Server_ip[$server]} "cat $file | grep -v '^#' | grep '/ \|CONFIG'| awk '{print \$6}'"> /tmp/start_num
      for strat in `cat /tmp/start_num`;
      do
	if [ `ssh dvctrader@${IND_Server_ip[$server]} "ps aux | grep trade_engine | grep $strat | grep -v grep | wc -l"` -eq 0 ]; then
	  echo "   $strat" >> /tmp/trade_engine_live_mail
	fi
      done
    done
    echo "" >> /tmp/trade_engine_live_mail
  #fi
done

if [ `cat /tmp/trade_engine_live_mail | wc -l` -gt 14 ]; then
  cat /tmp/trade_engine_live_mail | mailx -s " TRADE ENGINES NOT RUNNING ALERT " -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in
fi
