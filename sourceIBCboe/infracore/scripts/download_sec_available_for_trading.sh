#!/bin/bash
download_sec_list_file() {
  wget -N --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/content/equities/EQUITY_L.csv		
}

verify_sec_list() {
  : '
    get start of the day timestamp
    get modification time of downloaded file
  '
  start_timestamp=`date -d $today_ +"%s"`;
  mod_timestamp=`date -r EQUITY_L.csv  +"%s"`
  if [ $mod_timestamp -lt $start_timestamp  ];then
    echo "" | mailx -s "Failed downloading Securities List" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
    exit
  fi
}

today_=`date +"%Y%m%d"`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
  echo "NSE holiday..., exiting";
  exit
fi

cd /spare/local/tradeinfo/NSE_Files/SecuritiesAvailableForTrading/
download_sec_list_file
verify_sec_list

cat EQUITY_L.csv | awk -F "," '{if($3=="EQ"){print $1}}' > sec_list.csv_$today_

>/tmp/sec_mailfile
#get previous day
previous_working_day=`/home/pengine/prod/live_execs/update_date $today_ P A`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $previous_working_day T`
while [ $is_holiday = "1" ];
do
  previous_working_day=`/home/pengine/prod/live_execs/update_date $previous_working_day P A`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $previous_working_day T`
done

diff -u sec_list.csv_$previous_working_day sec_list.csv_$today_ | grep -v "\-\-\-" | grep -v "+++"   | grep -v @ > /tmp/sec_list_diff.txt

echo -e "=============== NEW SYMBOLS ==============" >> /tmp/sec_mailfile
grep "^+" /tmp/sec_list_diff.txt | awk '{print substr($1,2);}' >> /tmp/sec_mailfile
echo -e "\n=============  REMOVED SYMBOLS ==========" >> /tmp/sec_mailfile
grep "^-" /tmp/sec_list_diff.txt  | awk '{print substr($1,2);}' >> /tmp/sec_mailfile

cat /tmp/sec_mailfile | mailx -s "SECURITIES AVAILABLE FOR TRADING" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in

