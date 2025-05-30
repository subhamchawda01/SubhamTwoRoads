#!/bin/bash
LOG_DIR="/spare/local/ORSlogs/NSE_EQ/"
declare -A server_to_ip_map
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
loggin_file="/tmp/margin_monitoring_file"
server_to_ip_map=( ["IND16"]="10.23.227.81" \
                   ["IND17"]="10.23.227.82" \
                   ["IND23"]="10.23.227.72" \
                   ["IND18"]="10.23.227.83")

print_usage_and_exit () {
      echo "$0 sleep_time" ;
      exit ;
}



#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

sleep_time=$1

today_=`date +"%Y%m%d"`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    exit
fi

echo "Running at Sleep Time $sleep_time"
> $loggin_file
$send_slack_exec nseinfo DATA "TIME: \t\t SERVER \t Netmargin \t Grossmargin \t OverallMargin \t  Reject \t Worst" 
while :
do 
    echo -e "TIME: \t\t SERVER \t Netmargin \t Grossmargin \t OverallMargin \t  Reject \t Worst"
    echo -e "TIME: \t\t SERVER \t Netmargin \t Grossmargin \t OverallMargin \t  Reject \t Worst " >>$loggin_file
    overall_margin=`ssh 10.23.5.67 "tail -n1 /home/dvcinfra/important/CM_Margin/CM_margin_file_10s" | cut -d' ' -f2` 
    time_update=`date -d "+ 330 minutes" +'%H:%M:%S'`
    for server in "${!server_to_ip_map[@]}";
      do
      session=`ssh ${server_to_ip_map[$server]} "crontab -l" |grep -w "SmartDaemonController.sh ORS"  |grep "START"  | awk -F"START" '{print $1}' | awk '{print $NF}' | sort | uniq` ;
      ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/ors_control.pl NSE $session DUMPORSPNLMARGINSTATUS"
      SERVER_LOG_file="${LOG_DIR}/${session}/log.${today_}"
      margin_log=`ssh ${server_to_ip_map[$server]} "cat $SERVER_LOG_file | strings | grep 'GLOBAL_MARGIN_FACTOR' " | tail -n1`
      tot_gross=`echo $margin_log | cut -d' ' -f16 | cut -d'.' -f1`
      curr_gross=`echo $margin_log | cut -d' ' -f10| cut -d'.' -f1`
      curr_gross=$((curr_gross * 100))
      gross_per=$((curr_gross / tot_gross))
      tot_net=`echo $margin_log | cut -d' ' -f19 | cut -d'.' -f1`
      curr_net=`echo $margin_log | cut -d' ' -f13| cut -d'.' -f1`
      curr_net=$((curr_net * 100))
      net_per=$((curr_net / tot_net))
      reject=`ssh ${server_to_ip_map[$server]} "tail -n150 $SERVER_LOG_file" | grep -i rejec | wc -l`
      worst=`ssh ${server_to_ip_map[$server]} "tail -n150 $SERVER_LOG_file" | grep -i worst | wc -l`
      echo -e "${time_update} \t ${server} \t\t ${net_per}% \t\t ${gross_per}% \t\t  ${overall_margin}% \t\t ${reject} \t\t ${worst}" >>$loggin_file
      echo -e "${time_update} \t ${server} \t\t ${net_per}% \t\t ${gross_per}% \t\t  ${overall_margin}% \t\t ${reject} \t\t ${worst}"
      [[ $overall_margin -gt 30 || $gross_per -gt 50 || $reject -gt 0 || $worst -gt 0 ]] &&  $send_slack_exec nseinfo DATA "${time_update} \t ${server} \t\t ${net_per}% \t\t ${gross_per}% \t\t  ${overall_margin}% \t\t ${reject} \t\t ${worst}"
      done
      echo "========================================================================================================="
      echo "=========================================================================================================" >>$loggin_file
      sleep $sleep_time;
done 
