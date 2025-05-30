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
   echo "BSE Holiday. Exiting...";
   exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4};
next_working_day=`/home/pengine/prod/live_execs/update_date $YYYY$MM$DD N W`
NXT_DAY_YYYY=${next_working_day:0:4}
NXT_DAY_MM=${next_working_day:4:2}
NXT_DAY_DD=${next_working_day:6:2}
nxt_wor="${NXT_DAY_DD}${NXT_DAY_MM}${NXT_DAY_YYYY}"

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo  "NextDay: $next_working_day";
cd /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/
#no Run file already downloaded
[[ $1 == "FORCE" ]] && `rm -rf "fo_secban_${next_working_day}.csv"` && `rm -rf "BannedScrips-${DD}${MM}${YYYY}.csv"`
[[ -f "fo_secban_${next_working_day}.csv" ]]  && exit;

echo "Downloading from web"
wget https://www.bseindia.com/Download/Derivative/MWPL/BannedScrips-${DD}${MM}${YYYY}.zip
[[ ! -f "BannedScrips-${DD}${MM}${YYYY}.zip" ]]  && echo "BAN FILE NOT UPDATED BannedScrips-${DD}${MM}${YYYY}.zip" && exit;
unzip BannedScrips-${DD}${MM}${YYYY}.zip
rm BannedScrips-${DD}${MM}${YYYY}.zip
tail -n +2 /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv | cut -d',' -f4 > /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${next_working_day}.csv
chgrp infra "/spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv"
chgrp infra /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${next_working_day}.csv

year=${next_working_day:0:4}
month=$(date -d $next_working_day +"%b")
next_day=${next_working_day:6:2}

[[ "`grep -i "${DD}/${MM}/${YYYY}" BannedScrips-${DD}${MM}${YYYY}.csv | wc -l `" -ne "0" ]] && echo "" | mailx -s "BSE SECURITIES UNDER BAND FILE UPDATED" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in

if [ "`grep -i "${DD}/${MM}/${YYYY}" BannedScrips-${DD}${MM}${YYYY}.csv `" -eq "0" ]; then
   error_msg="SecBan File for $next_working_day is not latest. Please download the file later"
   echo "File Not Updated"
   $send_slack_exec prod-issues DATA "$error_msg"
fi
cd /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/
chgrp infra *
