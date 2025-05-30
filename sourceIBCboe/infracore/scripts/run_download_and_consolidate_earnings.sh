#!/bin/bash

GetNextWorkingDay() {
  dateyyyymmdd=$1;
  dateyyyymmdd=`/home/pengine/prod/live_execs/update_date $dateyyyymmdd N W`;
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $dateyyyymmdd T`
  while [ $is_holiday = "1" ];
  do
    dateyyyymmdd=`/home/pengine/prod/live_execs/update_date $dateyyyymmdd N W`;
    is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $dateyyyymmdd T`
  done 
}

today=`date +"%Y%m%d"`
/home/dvctrader/.conda/envs/env/bin/python /home/pengine/prod/live_scripts/download_earning_dates.py >/dev/null 2>/dev/null

status=$?

if [ $status -ne 0 ];
then
  echo "" | mailx -s "FAILED TO CONSOLIDATE EARNINGS REPORT" -r \
	  "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in \
	  raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
  exit
fi

earnings_file='/spare/local/tradeinfo/NSE_Files/EarningsReports/earnings.'${today}
consolidatedfile=/spare/local/tradeinfo/NSE_Files/EarningsReports/consolidated_earnings.csv
tmp_consolidatedfile=/spare/local/tradeinfo/NSE_Files/EarningsReports/.consolidated_earnings.csv


cat $earnings_file >> $consolidatedfile
cat $consolidatedfile | sort | uniq > $tmp_consolidatedfile

mv $tmp_consolidatedfile $consolidatedfile

scp $consolidatedfile  dvctrader@10.23.227.62:/spare/local/files/NSE/MidTermEarningsLogs/upcoming_earnings.csv

rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.69:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.65:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.64:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.81:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.84:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.63:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.62:/spare/local/tradeinfo --delete-after
