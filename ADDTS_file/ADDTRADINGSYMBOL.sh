#!/bin/bash

DATE=`date +%Y%m%d`
slack_exec=/home/pengine/prod/live_execs/send_slack_notification 
addts_config_file=/home/pengine/prod/live_configs/`hostname`"_"addts.cfg
ORS_DIR=`ps aux | grep cme | grep -v grep | awk '{print $15}'`
ORS_FILE="${ORS_DIR}/log.${DATE}"

declare -A server_to_profile_
server_to_profile_=( ["sdv-ind-srv14"]="MSFO7" \
                     ["sdv-ind-srv15"]="MSFO4" \
                     ["sdv-ind-srv16"]="MSEQ2" \
                     ["sdv-ind-srv17"]="MSEQ3" \
                     ["sdv-ind-srv18"]="MSEQ4" \
                     ["sdv-ind-srv19"]="MSFO5" \
                     ["sdv-ind-srv20"]="MSFO6" )

declare -A server_to_ip_map
server_to_ip_map=( ["sdv-ind-srv14"]="10.23.227.64" \
                   ["sdv-ind-srv15"]="10.23.227.65" \
                   ["sdv-ind-srv16"]="10.23.227.81" \
                   ["sdv-ind-srv17"]="10.23.227.82" \
                   ["sdv-ind-srv18"]="10.23.227.83" \
                   ["sdv-ind-srv19"]="10.23.227.69" \
                   ["sdv-ind-srv20"]="10.23.227.84" )

profile_=${server_to_profile_[`hostname`]}
server_ip=${server_to_ip_map[`hostname`]}
echo "$profile_ :: $server_ip"

if [ $# -ge 1 ] ; then 
  addts_config_file=$1 ;

  if [ ! -f $addts_config_file ] ; then 
    echo "ADDTS-FAILURE -> CAN'T READ $addts_config_file" ; 
    exit ;
  fi  

fi

[ -f $addts_config_file -a -s $addts_config_file -a -r $addts_config_file ] || `$slack_exec "prod-issues" "DATA" "ADDTS-FAILURE -> MISSING $addts_config_file"` ;

cat $addts_config_file | tr '"' ' ' > /tmp/addts_tmp_file_
chmod 777 /tmp/addts_tmp_file_
#chown dvcinfra:infra addts_tmp_file
#mv addts_tmp ${addts_config_file}

/home/pengine/prod/live_scripts/ors_control.pl NSE ${profile_} LOADTRADINGSYMBOLFILE /tmp/addts_tmp_file_
sleep 6
rm /tmp/addts_tmp_file_

ssh dvcinfra@${server_ip} "tail $ORS_FILE" >/tmp/ors_log_tail
if [ `cat /tmp/ors_log_tail | grep "Invalid Control Message received LOADTRADINGSYMBOLFILE" | head -1 | wc -l` == "1" ]; then
  echo "do new addts"
  tail ${ORS_FILE} | mailx -s "=== FILE ADDTS FAILED ON ${HOSTNAME} ===" -r "${HOSTNAME}-${USER}<hardik.dhakate@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in , raghunandan.sharma@tworoads-trading.co.in  #, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in
  
  rm /tmp/ors_log_tail
  while read line 
  do
    if [[  $( echo $line | head -c 1 ) != '#'  &&  $(echo "$line" | wc -w) -ge 7  ]] ;
    then
      /home/pengine/prod/live_scripts/ors_control.pl $line ;
    fi 
  done < $addts_config_file
fi
rm /tmp/ors_log_tail

