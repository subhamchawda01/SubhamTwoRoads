#!/bin/bash
TMP_FILE='/tmp/file_margin_update.txt'
rm $TMP_FILE


command_="curl 'https://www.connect2nsccl.com/margin-summary/get-dflt-srvc-data' -H 'User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:95.0) Gecko/20100101 Firefox/95.0' -H 'Accept: application/json' -H 'Accept-Language: en-US,en;q=0.5' -H 'Accept-Encoding: gzip, deflate, br' -H 'Content-Type: application/json' -H 'Cache-Control: no-cache' -H 'Pragma: no-cache' -H 'ufp: 0658fa949957dc717715183309449e05' -H 'Connection: keep-alive' -H 'Referer: https://www.connect2nsccl.com/margin-summary/' -H 'Cookie: bowfp=0658fa949957dc717715183309449e05; srvt=xL643UjA5Fhaitk3X7ymHsnM5yj5hZH6Pcaz8NyNY5MMkYHaUjNwIs2IcanQ2Rwg' -H 'Sec-Fetch-Dest: empty' -H 'Sec-Fetch-Mode: cors' -H 'Sec-Fetch-Site: same-origin'"



command_=""

>/home/dvcinfra/important/CM_Margin/CM_margin_file_10s
echo "tail -f /home/dvcinfra/important/CM_Margin/CM_margin_file_10s"
echo 
while true
do
  today_=`date +"%Y%m%d"`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
  if [ $is_holiday = "1" ];then
     echo "NSE holiday..., exiting";
     exit
  fi
  ssh tarunjoshi@10.23.5.58  $command_ > $TMP_FILE

  grep -iq "Invalid" $TMP_FILE
  if [ $? -eq 0 ] ;then
    echo ""| mailx -s "CM login In To https://www.connect2nsccl.com/" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in tarun.joshi@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in

     exit;
  fi

  date_=`date +"%Y%m%d"`
  time_=`date -d "+ 330 minutes" +'%H:%M:%S'`
  HHMM=`date +"%H%M"`

###  margin=`cut -d',' -f11 $TMP_FILE | cut -d':' -f2 |  head -1|cut -d'.' -f1`
  margin=`cat $TMP_FILE | cut -d':' -f13 | cut -d '.' -f1`
  echo "Current Margin $margin"
  echo "$time_ $margin" >> /home/dvcinfra/important/CM_Margin/CM_margin_file_10s

  if [ $margin -lt '0' ] || [ $margin -gt '100' ]; then
      continue;
  fi

  [[ $margin -gt '40' ]] && { echo "" | mailx -s "CM MARGIN ALERT ABOVE 40 $margin $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in; }
  [[ $margin -gt '65' ]] && { echo "" | mailx -s "CM MARGIN ALERT ABOVE 65 $margin $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in akshay.tk@tworoads-trading.co.in nagaraj.aithal@tworoads.co.in nseall@tworoads.co.in; $send_slack_exec nseinfo DATA "CM Margin-$margin";} 
  sleep 15s
done
