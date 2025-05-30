#!/bin/bash

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

HHMM=`date +"%H%M"`
YYYYMMDD=`date +"%Y%m%d"`
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
if [ ${HHMM} -lt 1000 ];
then
  GetPreviousWorkingDay;
fi
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4};
next_working_day=`/home/pengine/prod/live_execs/update_date $YYYY$MM$DD N W`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo  "NextDay: $next_working_day";
cd /spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/
#no Run file already downloaded
[[ $1 == "FORCE" ]] && `rm -rf "fo_secban_${next_working_day}.csv"`
[[ -f "fo_secban_${next_working_day}.csv" ]]  && exit;
rm fo_secban.csv
wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/content/fo/fo_secban.csv
grep -v "Securities" fo_secban.csv | awk -F"," '{print $2}' > fo_secban_$next_working_day".csv"
#Fixing invisible character below
cat fo_secban_$next_working_day".csv" | sed 's/\r$//' > fo_secban_$next_working_day".csv_temp"
mv fo_secban_$next_working_day".csv_temp" fo_secban_$next_working_day".csv"

year=${next_working_day:0:4}
month=$(date -d $next_working_day +"%b")
next_day=${next_working_day:6:2}

[[ "`grep -i "$next_day.*$month.*$year" fo_secban.csv | wc -l `" -ne "0" ]] && echo "" | mailx -s "SECURITIES UNDER BAND FILE UPDATED" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in

if [ "`grep -i "$next_day.*$month.*$year" fo_secban.csv | wc -l `" -eq "0" ]; then
   error_msg="SecBan File for $next_working_day is not latest. Please download the file later"
   $send_slack_exec prod-issues DATA "$error_msg"
fi

