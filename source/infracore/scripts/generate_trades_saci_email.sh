# sends email for 16,17,18 open position
#!/bin/bash
date=`date +\%Y\%m\%d`
mail_file=/tmp/saci_trade_position_report_email.txt
temp_mail_file=/tmp/saci_trade_position_report.txt
>$mail_file

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

send_mail() {
mailx -s "START RATIO POSITION ALERT $date" -r  "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in < $mail_file
#mailx -s "START RATIO POSITION ALERT $date" -r raghunandan.sharma@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in  < $mail_file
}

declare -A server_ip_map
server_ip_map=( ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83")
#main
for server in "${!server_ip_map[@]}";
do
      ssh ${server_ip_map[$server]} "/home/pengine/prod/live_scripts/generate_trades_saci.sh \> /dev/null 2\>\&1"
      scp "dvctrader@"${server_ip_map[$server]}":"$temp_mail_file $temp_mail_file
      less $temp_mail_file >>$mail_file
      echo >>$mail_file
done

send_mail

