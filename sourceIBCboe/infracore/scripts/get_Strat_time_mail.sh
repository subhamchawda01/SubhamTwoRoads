#!/bin/bash

strat_time_mail_file="/tmp/strat_time_mail"
>${strat_time_mail_file}
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
  echo ""
  ors_time_=`ssh ${IND_Server_ip[$server]} "ps aux | grep cme_ | grep -v grep"  | awk '{print $9}'`
  hr_=`echo $ors_time_ | cut -d: -f1`
  min_=`echo $ors_time_ | cut -d: -f2`
#ssh ${IND_Server_ip[$server]} "ps aux | grep trade_eng | grep -v grep" | awk '{print $9,$(NF)}' | sed 's/:/ /g'
  trade_engine_=`ssh ${IND_Server_ip[$server]} "ps aux | grep trade_eng | grep -v grep" | awk '{print $9,$(NF)}' | sed 's/:/ /g' | awk -v hr=$hr_ -v min=$min_ '{if(hr > $1 || (hr == $1 && min > $2)) print $0}'`
  echo -e "for ---- $server : \n$ors_time_ : $hr_ $min_";
  if [ ! -z "$trade_engine_" ] ; then 
    echo "DONE"
    echo -e "$server:\n$trade_engine_\n" >> $strat_time_mail_file
  else
    echo "EMPTY"
  fi
#echo $ors_time_;
done

if [ `cat $strat_time_mail_file | wc -l` -gt 0 ]; then
  cat $strat_time_mail_file | mailx -s "=== ALERT: Strat Start Time ===" -r "${HOSTNAME}-${USER}<hardik.dhakate@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in , raghunandan.sharma@tworoads-trading.co.in , subham.chawda@tworoads-trading.co.in #, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in
fi
