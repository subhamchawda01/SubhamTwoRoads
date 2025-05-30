#!/bin/bash
TMP_FILE='/tmp/file_margin_update.txt'
rm $TMP_FILE
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
#$send_slack_exec prod-issues DATA "$error_msg"

command_="curl 'https://www.connect2nsccl.com/risk-fo/tm-margins-for-tm/JSON:JSON' -H 'Pragma: no-cache' -H 'ufp: c3977dc9294262d4fac53d0fbd86c243' -H 'Sec-Fetch-Site: same-origin' -H 'Accept-Encoding: gzip, deflate, br' -H 'Accept-Language: en-US,en;q=0.9' -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/76.0.3809.100 Safari/537.36' -H 'Sec-Fetch-Mode: cors' -H 'Content-Type: application/json' -H 'Accept: application/json' -H 'Cache-Control: no-cache' -H 'Referer: https://www.connect2nsccl.com/member-dashboard/' -H 'Cookie: bowfp=c3977dc9294262d4fac53d0fbd86c243; bm_mi=7434B5D1F2E1BBF0BB86A13D4AFAC210~TQDPHhn80d24fH354DkOAXf9KxYrxUSaoOvx/9aC/0qhJ/1FIii6fN51I/clE10m5kFsv7KPeQ7V565EcXNZ+ATexWNKZhc0ScDh/qM4mo30uyngIB599S19RhipWspE2t203kUohRq/8XCU77foTb3QrZIhzZGxxsr995ERroGTJE3OScVm1X4Y81xxZ/4erTnzMQKdqwBXg36M38fuZ3QskP0gM7x25iDApqzJj2fNXP7/7161NEUU/GQvSICSCZN2JYbpMANlyYDWwDA0Irzwz1qGZrfTFFd2j8Y852Y=; ak_bmsc=7A34D787A751C14E710ED792BF178F9C172B30DCB93500003534EC5E03912E64~plYCfe1ECWaY1cUAWHwv3p8AeVW0ZoaRXN9bWZXo/ihf0Vfktnf+MnusNmgYb86a3TIGtedFqPNnsVl7QdYqkSdlBtoc9hhJZyzvXr0zLbj37okqXZl1R0DidTfu9Wp4LTcqHZKDa9WEnoI5C9cdtWJqO8copHTetaOlUWPmp6lbCO0Dq0bUcLf1jY+SYPj/Tw/JQ8Lm8Bq5AmdSBg2bJ+ifeeLkJeOW0htZDvVvKeuvaqZU7QZj6DIwA2CPTtXLP2; srvt=ku_MXzRuvulSbexsrqpi-6sW7DQimbA7mU8jWi2y8vwPxIp5X7Jfz6c5uXkdoIe2; bm_sv=11330741F7CEB7E40BA1D2700218EFE2~o3iEGbji5Q1s/ZlD5uCcBqJANioCx0vBK9TIRErU6G87QNEtfNvza9oxLXY3TfAlpIKImaHyrWUd3bsAgZWOKnzzCgAthJzrgk91wVYZqwGVLEEMknoF2YTYjNH1In40gTxJv6P5rpC8kqcwpP7Rq+vNm4YtaA9yFspNYvY05NM=' -H 'Connection: keep-alive' --compressed"







today_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
   echo "NSE holiday..., exiting";
    exit
fi
ssh raghu@10.23.5.27  $command_ > $TMP_FILE

grep -iq "Invalid" $TMP_FILE
if [ $? -eq 0 ] ;then
  echo ""| mailx -s "log In To https://www.connect2nsccl.com/" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in 
  exit;
fi

date_=`date +"%Y%m%d"`
time_=`date -d "+ 330 minutes" +'%H:%M'`
HHMM=`date +"%H%M"`

margin=`cut -d',' -f11 $TMP_FILE | cut -d':' -f2 |  head -1|cut -d'.' -f1`
file_ge='/home/dvcinfra/important/Generate_Margin_Summary/gen_margin_data/'

if [[ ! -f ${file_ge}${date_} ]]; then
      $send_slack_exec nseinfo DATA "FO Margin Session intialised"
fi
echo "$time_ $margin" >> ${file_ge}${date_}

/home/dvcinfra/important/Generate_Margin_Summary/scripts/generate_margin_report.sh
echo "margin $margin"
[[ $margin -gt '80' ]] && { echo "" | mailx -s "FO MARGIN ALERT ABOVE 80 $margin $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in akshay.tk@tworoads-trading.co.in nagaraj.aithal@tworoads.co.in; $send_slack_exec production-issues DATA "FO Margin-$margin";  exit; }
[[ $margin -gt '75' ]] && { echo "" | mailx -s "FO MARGIN ALERT ABOVE 75 $margin $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in akshay.tk@tworoads-trading.co.in nagaraj.aithal@tworoads.co.in; $send_slack_exec nseinfo DATA "FO Margin-$margin"; exit; }
