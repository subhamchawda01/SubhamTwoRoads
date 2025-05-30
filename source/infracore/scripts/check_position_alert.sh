#!/bin/bash

email_file="/tmp/email_alert_file_position"
>$email_file

declare -A server_to_ip_map

server_to_ip_map=( ["IND16"]="10.23.227.81" \
                    ["IND17"]="10.23.227.82" \
                    ["IND18"]="10.23.227.83" \
		    ["IND25"]="10.23.227.75" \
		    ["IND23"]="10.23.227.72" )
#	 	    ["IND22"]="10.23.227.71")

today=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday = "1" ];then
        echo "NSE holiday..., exiting";
        exit
fi

for server in "${!server_to_ip_map[@]}";
do
  tmp_run_ser="/tmp/run_$server"
  tmp_server_run="/tmp/live_file_path_strat${server}.txt"
  tmp_serverdic_run="/tmp/live_file_dic${server}.txt"
  tmp_start_posset="/tmp/server_start_position_set"
  tmp_other_posset="/tmp/server_postion_seted"
  check_product="/tmp/product_having_pos_zerro"
  rm -f $tmp_run_ser
  >$tmp_start_posset
  >$tmp_other_posset
  >$check_product
  echo $server  ${server_to_ip_map[$server]}
  scp dvctrader@${server_to_ip_map[$server]}:/home/dvctrader/ATHENA/run.sh $tmp_run_ser
  if [ $? -ne 0 ];
      then
          echo "${server} => scp failed run scipt to check  " >> $email_file
          continue
  fi
  scp dvctrader@${server_to_ip_map[$server]}:"/spare/local/files/NSE/PositionLimits/limits.${today}_start" $tmp_start_posset
  ssh dvctrader@${server_to_ip_map[$server]} "cat /spare/local/files/NSE/PositionLimits/limits.${today} /spare/local/files/NSE/PositionLimits/limits.${today}_1" >$tmp_other_posset
  cat $tmp_run_ser | grep -v "^#" |grep LIVE | grep -v NON_FO_MIDTERM | cut -d' ' -f2 >$tmp_server_run
  >$tmp_serverdic_run
  for file in `cat $tmp_server_run`;
  do
       echo $(dirname "${file}") >>$tmp_serverdic_run
  done
  sort $tmp_serverdic_run | uniq >$tmp_server_run
  mv $tmp_server_run $tmp_serverdic_run
  for dir in `cat $tmp_serverdic_run`;
    do
          echo "$server in $dir "
          echo "$server in $dir " >>$email_file
          ssh dvctrader@${server_to_ip_map[$server]} "grep ' = 0' $dir/PositionLimits.csv |  cut -d' ' -f1 | cut -d'_' -f2  | sort | uniq" >$check_product
          if [[ $dir == *"START_RATIO"* ]];then
              for prod in `cat $check_product`;do
                  prod=${prod//&/\\&}
                  if ! grep -q $prod "$tmp_start_posset"; then
                      echo "$prod " >>$email_file
                  fi 
              done
          else 
              for prod in `cat $check_product`;do
                  prod=${prod//&/\\&}
                  if ! grep -q $prod "$tmp_other_posset"; then
                      echo "$prod " >>$email_file
                  fi 
              done
          fi
    done
done

cat $email_file | mailx -s "Position Limit alert CASH $today" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" ravi.parikh@tworoads.co.in, infra_alerts@tworoads-trading.co.in, raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in
