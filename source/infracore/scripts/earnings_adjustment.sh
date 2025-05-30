#!/bin/bash
today_=`date +"%Y%m%d"`
today_tmp=`date +"%Y-%m-%d"`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
	echo "NSE holiday..., exiting";
	exit
fi

cd /spare/local/tradeinfo/NSE_Files/EarningsReports


date_to_grep=${today_tmp}
prev_day=`/home/pengine/prod/live_execs/update_date ${today_} P A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ${prev_day} T`
while [ $is_holiday = "1" ];
do
  date_to_grep=${date_to_grep}"|"${prev_day:0:4}"-"${prev_day:4:2}"-"${prev_day:6:2};
  prev_day=`/home/pengine/prod/live_execs/update_date ${prev_day} P A`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ${prev_day} T`
done

>/spare/local/files/NSE/PositionLimits/limits.${today_}
for prod in `egrep "${date_to_grep}" consolidated_earnings.csv | awk '{print $1}'`;
do
  for key in `grep "NSE_${prod}_" /home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_SECTOR_SPC/PositionLimits.csv | awk -F "=" '{print $1}'`;
  do
    grep "${key}" /home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_SECTOR_SPC/PositionLimits.csv >> /spare/local/files/NSE/PositionLimits/limits.${today_}
    key=${key//&/\\&}
    sed -i "s/$key = .*/$key = 0/g"  /home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_SECTOR_SPC/PositionLimits.csv
  done
done

#send mail with list of symbols and their position limits
>/tmp/earnings_mail_file

echo "=============== SYMBOLS ==============" >/tmp/earnings_mail_file

egrep "${date_to_grep}"  consolidated_earnings.csv | awk 'BEGIN{FS=","}{print $1,$2}' >>/tmp/earnings_mail_file 

echo -e "\n" >>/tmp/earnings_mail_file

echo "============== Position Limits =======" >>/tmp/earnings_mail_file

for prod in `egrep "${date_to_grep}" consolidated_earnings.csv | awk 'BEGIN{FS=","}{print $1}'`;do
  grep "NSE_${prod}_"  /home/dvctrader/ATHENA/CONFIG_FUT0_VWAP_SECTOR_SPC/PositionLimits.csv >>/tmp/earnings_mail_file
done

cat /tmp/earnings_mail_file | mailx -s "Earnings Adjustment - IND19" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in
#cat /tmp/earnings_mail_file | mailx -s "Earnings Adjustment - IND19" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in

/home/pengine/prod/live_execs/user_msg --traderid  123495 --setmaxpos 1
