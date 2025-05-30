#!/bin/bash

#End Date
if [ $# -ne 2 ] ; then
  echo "Called As : " $* ;
  echo "$0 startDate ENDDATE[YYYYMMDD]" ;
  exit;
fi

#start Date
date_=$1
end_date_=$2
expiry_file="/spare/local/expiry_dates"
mail_tmp="/tmp/mails_index_worker_from_idx_data_month_week"

>$mail_tmp
echo "STARTDATE: $date_ ENDDATE: $end_date_ OUTPUTPATH: $OUTPUT_PATH"

while [ $date_ -le $end_date_ ]; do
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
  if [ $is_holiday = "1" ] ; then
   echo "$date_ NSE Holiday. Exiting...";
   date_=`date -d "$date_ +1 day" +"%Y%m%d"`
   continue;
  fi
  echo "Running For Date $date_";
  echo $date_ >>$mail_tmp
  yesterday=$date_
  today_start_time=`date -d"$yesterday" +"%s"`
  today_end_time=$((today_start_time+35940));
  c1=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA/MONTHLYOPT/NIFTY" | wc -l`
  c2=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA/MONTHLYOPT/BANKNIFTY" | wc -l`
  c3=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA_TUES/MONTHLYOPT/FINNIFTY" | wc -l`
  c4=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA_TUES/MONTHLYOPT/MIDCPNIFTY" | wc -l`
  echo "MONTHLY   NIFTY: $c1  BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4"
  echo "MONTHLY   NIFTY: $c1  BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4" >> $mail_tmp
  c1=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA/WEEKLYOPT/NIFTY" | wc -l`
  c2=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA/WEEKLYOPT/BANKNIFTY" | wc -l`
  c3=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA_TUES/WEEKLYOPT/FINNIFTY" | wc -l`
  c4=`ssh dvctrader@54.90.155.232 "grep $today_end_time /spare/local/INDEX_BARDATA_TUES/WEEKLYOPT/MIDCPNIFTY" | wc -l`
  echo "WEEKLY   NIFTY: $c1   BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4"
  echo "WEEKLY   NIFTY: $c1   BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4" >> $mail_tmp
  date_=`date -d "$date_ +1 day" +"%Y%m%d"`
done

#echo $mail_tmp | mailx -s "Idx BarData For $today" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in
