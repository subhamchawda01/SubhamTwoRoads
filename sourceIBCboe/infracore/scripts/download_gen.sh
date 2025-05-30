#!/bin/bash
TMP_FILE='/tmp/file_margin_update.txt'
rm $TMP_FILE
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
#$send_slack_exec prod-issues DATA "$error_msg"


command_="curl 'https://www.connect2nsccl.com/margin-summary/get-dflt-srvc-data' -H 'User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:95.0) Gecko/20100101 Firefox/95.0' -H 'Accept: application/json' -H 'Accept-Language: en-US,en;q=0.5' -H 'Accept-Encoding: gzip, deflate, br' -H 'Content-Type: application/json' -H 'Cache-Control: no-cache, max-age=0' -H 'Pragma: no-cache' -H 'ufp: 46ace8f1a081e287c3af8618f771475d' -H 'Connection: keep-alive' -H 'Referer: https://www.connect2nsccl.com/margin-summary/' -H 'Cookie: bowfp=0658fa949957dc717715183309449e05; srvt=dEjAHFyKT1b6nxUhL3A3WxqKUhr3GrziMibq69WpBQaUbBP6VO6D9cxdp2TZvO4W' -H 'Sec-Fetch-Dest: empty' -H 'Sec-Fetch-Mode: cors' -H 'Sec-Fetch-Site: same-origin'"

today_=`date +"%Y%m%d"`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
   echo "NSE holiday..., exiting";
    exit
fi
ssh tarunjoshi@10.23.5.58  $command_ > $TMP_FILE

grep -iq "Invalid" $TMP_FILE
if [ $? -eq 0 ] ;then
  echo ""| mailx -s "log In To https://www.connect2nsccl.com/" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in tarun.joshi@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in

  exit;
fi

date_=`date +"%Y%m%d"`
time_=`date -d "+ 330 minutes" +'%H:%M'`
HHMM=`date +"%H%M"`

######margin=`cut -d',' -f11 $TMP_FILE | cut -d':' -f2 |  head -1|cut -d'.' -f1`
margin=`cat $TMP_FILE  | cut -d':' -f52 | cut -d '.' -f1`
echo "Current Margin: $margin"
file_ge='/home/dvcinfra/important/Generate_Margin_Summary/gen_margin_data/'

if [[ ! -f ${file_ge}${date_} ]]; then
      $send_slack_exec nseinfo DATA "FO Margin Session intialised TIME:$time_ MAR: $margin"
fi
echo "$time_ $margin" >> ${file_ge}${date_}

/home/dvcinfra/important/Generate_Margin_Summary/scripts/generate_margin_report.sh
echo "margin $margin"
[[ $margin -gt '75' ]] && { echo "" | mailx -s "FO MARGIN ALERT ABOVE 75 $margin $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in akshay.tk@tworoads-trading.co.in nagaraj.aithal@tworoads.co.in nseall@tworoads.co.in smit@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in; $send_slack_exec production-issues DATA "FO Margin-$margin";  exit; }
[[ $margin -gt '70' ]] && { echo "" | mailx -s "FO MARGIN ALERT ABOVE 70 $margin $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in akshay.tk@tworoads-trading.co.in nagaraj.aithal@tworoads.co.in infra_alerts@tworoads-trading.co.in; $send_slack_exec nseinfo DATA "FO Margin-$margin"; exit; }
