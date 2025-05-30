#!/bin/bash
LOG_DIR="/spare/local/ORSlogs/NSE_EQ/"
declare -A server_to_ip_map
loggin_file="/tmp/margin_monitoring_file_all_ind"
server_to_ip_map=( ["IND11"]="10.23.227.61" \
                   ["IND14"]="10.23.227.64" \
                   ["IND15"]="10.23.227.65" \
                   ["IND16"]="10.23.227.81" \
                   ["IND17"]="10.23.227.82" \
                   ["IND18"]="10.23.227.83" \
                   ["IND19"]="10.23.227.69" \
                   ["IND20"]="10.23.227.84" \
                   ["IND22"]="10.23.227.71" \
                   ["IND23"]="10.23.227.72" \
                   ["INDB11"]="192.168.132.11" \
                   ["INDB12"]="192.168.132.12")

today_=`date +"%Y%m%d"`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    exit
fi
> $loggin_file
echo -e "SERVER \t\t Netmargin \t\t Grossmargin \t\tPnl";
echo -e "SERVER \t   Netmargin \t  Grossmargin \t  Pnl" >>$loggin_file
for server in "${!server_to_ip_map[@]}";
do
      session=`ssh ${server_to_ip_map[$server]} "crontab -l" | grep -w "SmartDaemonController.sh ORS"  | grep "START"  | awk -F"START" '{print $1}' | awk '{print $NF}' | sort | uniq` ;
      if [[ $server == "IND11" ]] || [[ $server == "IND14" ]] || [[ $server == "IND15" ]] || [[ $server == "IND19" ]] || [[ $server == "IND20" ]] || [[ $server == "IND22" ]]; then
        LOG_DIR="/spare/local/ORSlogs/NSE_FO/";
        ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/ors_control.pl NSE $session DUMPORSPNLMARGINSTATUS"
      elif [[ $server == "INDB11" ]] || [[ $server == "INDB12" ]]; then
        ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/ors_control.pl BSE $session DUMPORSPNLMARGINSTATUS"
        LOG_DIR="/spare/local/ORSlogs/BSE_EQ/";
      else
        ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/ors_control.pl NSE $session DUMPORSPNLMARGINSTATUS"
        LOG_DIR="/spare/local/ORSlogs/NSE_EQ/";
      fi
      SERVER_LOG_file="${LOG_DIR}/${session}/log.${today_}"
      margin_log=`ssh ${server_to_ip_map[$server]} "cat $SERVER_LOG_file | strings | grep 'GLOBAL_MARGIN_FACTOR' " | tail -n1`
      pnl_log=`ssh ${server_to_ip_map[$server]} "cat $SERVER_LOG_file | strings | grep DumpORSPnlStatus | grep TOTAL"| tail -n1`
      tot_gross=`echo $margin_log | cut -d' ' -f16 | cut -d'.' -f1`
      curr_gross=`echo $margin_log | cut -d' ' -f10| cut -d'.' -f1`
      curr_gross=$((curr_gross * 100))
      gross_per=$((curr_gross / tot_gross))
      tot_net=`echo $margin_log | cut -d' ' -f19 | cut -d'.' -f1`
      curr_net=`echo $margin_log | cut -d' ' -f13| cut -d'.' -f1`
      curr_net=$((curr_net * 100))
      net_per=$((curr_net / tot_net))
      tot_pnl=`echo $pnl_log | cut -d' ' -f10 | cut -d'-' -f2 | cut -d'.' -f1`
      curr_pnl=`echo $pnl_log | cut -d' ' -f7| cut -d'.' -f1`
      curr_pnl=$((curr_pnl * 100))
      pnl_per=$((curr_pnl / tot_pnl))
      echo -e "${server} \t\t ${net_per}% \t\t ${gross_per}% \t\t  ${pnl_per}%" >>$loggin_file
      echo -e "${server} \t\t ${net_per}% \t\t ${gross_per}% \t\t  ${pnl_per}%"
      [[ $net_per -gt 70 || $gross_per -gt 70 ]] &&  $send_slack_exec nseinfo DATA "${time_update} \t ${server} \t\t ${net_per}% \t\t ${gross_per}% \t\t  ${pnl_per}"
done

cat $loggin_file | mailx -s "Margin and Pnl details IND" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in, ravi.parikh@tworoads.co.in 
slack_exec="/home/pengine/prod//live_execs/send_slack_notification"
slack_channel="mail-service"
$slack_exec $slack_channel DATA "*${HOSTNAME}-${USER}*\n`cat $loggin_file `\n"
