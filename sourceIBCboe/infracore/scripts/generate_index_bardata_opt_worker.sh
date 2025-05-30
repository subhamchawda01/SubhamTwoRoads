#!/bin/bash
mail_file="/tmp/file_user_to_indx_bardata"
>$mail_file

#End Date
if [ $# -ne 3 ] ; then
  echo "Called As : " $* ;
  echo "$0 startDate ENDDATE[YYYYMMDD]" ;
  exit;
fi

#start Date
date_=$1
end_date_=$2
outpath_=$3
outpath_tues='/spare/local/INDEX_BARDATA_TUES/'

echo "STARTDATE: $date_ ENDDATE: $end_date_ OUTPUTPATH: $OUTPUT_PATH"
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "$date_ NSE Holiday. Exiting...";
   exit
fi

echo "Running For Date $date_";
ssh dvctrader@52.90.0.239 "/home/dvctrader/stable_exec/scripts/generate_historical_bardata_indx.sh $date_ $end_date_ $outpath_" >/tmp/logs_opt_indx_data_generate_bar 2>&1
sleep 60;
Instance=`ssh dvctrader@52.90.0.239 "ps aux | grep 'nse_historical_bar_data_generator' | grep -v grep" | wc -l`
echo "Instance: $Instance"
while [[ $Instance -gt 0 ]];
do
  sleep 60;
  Instance=`ssh dvctrader@52.90.0.239 "ps aux | grep 'nse_historical_bar_data_generator' | grep -v grep" | wc -l`
  echo "Status: $Instance"
done

echo "Running For Date MID AND FIN $date_";
ssh dvctrader@52.90.0.239 "/home/dvctrader/stable_exec/scripts/generate_historical_bardata_indx_tuesday.sh $date_ $end_date_ $outpath_tues" >/tmp/logs_opt_indx_data_generate_bar_tuesday 2>&1
sleep 60;
Instance=`ssh dvctrader@52.90.0.239 "ps aux | grep 'nse_historical_bar_data_generator' | grep -v grep" | wc -l`
echo "Instance: $Instance"
while [[ $Instance -gt 0 ]];
do
  sleep 60;
  Instance=`ssh dvctrader@52.90.0.239 "ps aux | grep 'nse_historical_bar_data_generator' | grep -v grep" | wc -l`
  echo "Status: $Instance"
done



yesterday=$date_
today_start_time=`date -d"$yesterday" +"%s"`
today_end_time=$((today_start_time+35940));
c1=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA/MONTHLYOPT/NIFTY | grep $today_end_time " | wc -l`
c2=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA/MONTHLYOPT/BANKNIFTY | grep $today_end_time " | wc -l`
c3=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA_TUES/MONTHLYOPT/FINNIFTY | grep $today_end_time " | wc -l`
c4=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA_TUES/MONTHLYOPT/MIDCPNIFTY | grep $today_end_time " | wc -l`
echo "MONTHLY   NIFTY: $c1  BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4"
echo "MONTHLY   NIFTY: $c1  BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4" >> $mail_file
c1=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA/WEEKLYOPT/NIFTY | grep $today_end_time " | wc -l`
c2=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA/WEEKLYOPT/BANKNIFTY | grep $today_end_time " | wc -l`
c3=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA_TUES/WEEKLYOPT/FINNIFTY | grep $today_end_time " | wc -l`
c4=`ssh dvctrader@52.90.0.239 "tail -n1000 /spare/local/INDEX_BARDATA_TUES/WEEKLYOPT/MIDCPNIFTY | grep $today_end_time " | wc -l`
echo "WEEKLY   NIFTY: $c1   BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4"
echo "WEEKLY   NIFTY: $c1   BANKNIFTY $c2 FINNIFTY: $c3 MIDCPNIFTY: $c4" >> $mail_file

cat $mail_file |  mailx -s "Index Opt BarData $today" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
