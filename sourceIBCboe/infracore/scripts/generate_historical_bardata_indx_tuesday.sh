#!/bin/bash

#End Date
if [ $# -ne 3 ] ; then
  echo "Called As : " $* ;
  echo "$0 startDate ENDDATE[YYYYMMDD] OUTPUTPATH" ;
  exit;
fi

#start Date
date_=$1
end_date_=$2
OUTPUT_PATH="$3/"
FILE_PROD_opt_fin="/spare/local/FILES/finnifty_shortcode_options_monthly"
FILE_PROD_opt_mid="/spare/local/FILES/midcp_shortcode_options_monthly"
FILE_PROD_opt_fin_w="/spare/local/FILES/finnifty_shortcode_options_weekly"
FILE_PROD_opt_mid_w="/spare/local/FILES/midcp_shortcode_options_weekly"

exec_bardata="/home/dvctrader/stable_exec/nse_historical_bar_data_generator_tuesday"
tmp_outpath="/spare/local/INDEX_BARDATA_TUES_temp/"
exec_fixbardata="/home/dvctrader/stable_exec/bardata_fix_for_weekly_expiry_tuesday"

#exec_bardata="/NAS4/raghu/Project/cvquant_install/basetrade/bin/nse_historical_bar_data_generator"
#exec_fixbardata="/home/dvctrader/stable_exec/bardata_fix_for_weekly_expiry"


mkdir -p $tmp_outpath/WEEKLYOPT
mkdir -p $tmp_outpath/MONTHLYOPT
mkdir -p $tmp_outpath/WEEKLYOPT_2

echo "STARTDATE: $date_ ENDDATE: $end_date_ OUTPUTPATH: $OUTPUT_PATH"
echo "REMOVE CORRUPT WEEKLY DATA"
mkdir -p $OUTPUT_PATH/WEEKLYOPT/
mkdir -p $OUTPUT_PATH/MONTHLYOPT/

start_=$(date -d "$date_ 0345" +%s);
for prod in `ls $OUTPUT_PATH/WEEKLYOPT/`;
do
  lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' "$OUTPUT_PATH/WEEKLYOPT/${prod}");
#echo "$prod $lineNum" ; 
  if [ -z "$lineNum" ]
  then
    echo "no corrupt data for $prod"
  else
    echo "corrupted data for $prod"
    sed -i "$lineNum,\$d" "$OUTPUT_PATH/WEEKLYOPT/${prod}"
  fi
done
echo "REMOVE CORRUPT MONTHLY DATA"
start_=$(date -d "$date_ 0345" +%s);
for prod in `ls $OUTPUT_PATH/MONTHLYOPT/`;
do
  lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' "$OUTPUT_PATH/MONTHLYOPT/${prod}");
#echo "$prod $lineNum" ; 
  if [ -z "$lineNum" ]
  then
    echo "no corrupt data for $prod"
  else
    echo "corrupted data for $prod"
    sed -i "$lineNum,\$d" "$OUTPUT_PATH/MONTHLYOPT/${prod}"
  fi
done

while [ $date_ -le $end_date_ ]; do
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
  if [ $is_holiday = "1" ] ; then
   echo "$date_ NSE Holiday. Exiting...";
   date_=`date -d "$date_ +1 day" +"%Y%m%d"`
   continue;
  fi
  echo "Running For Date $date_";
  >$tmp_outpath/MONTHLYOPT/MIDCPNIFTY
  >$tmp_outpath/MONTHLYOPT/FINNIFTY
  echo "$exec_bardata $date_ $FILE_PROD_opt_fin IST_915 IST_1530  1 $tmp_outpath/MONTHLYOPT/"
  taskset -c 20,21,22 $exec_bardata $date_ $FILE_PROD_opt_fin IST_915 IST_1530  1 "$tmp_outpath/MONTHLYOPT/" &
  echo "$exec_bardata $date_ $FILE_PROD_opt_mid IST_915 IST_1530  1 $tmp_outpath/MONTHLYOPT/"
  taskset -c 23,24,25 $exec_bardata $date_ $FILE_PROD_opt_mid IST_915 IST_1530  1 "$tmp_outpath/MONTHLYOPT/" &
  
  >$tmp_outpath/WEEKLYOPT/MIDCPNIFTY
  >$tmp_outpath/WEEKLYOPT/FINNIFTY
  echo "Running Normally weekly"
  echo "$exec_bardata $date_ $FILE_PROD_opt_fin_w IST_915 IST_1530  1 $tmp_outpath/WEEKLYOPT/"
  taskset -c 26,27,28 $exec_bardata $date_ $FILE_PROD_opt_fin_w IST_915 IST_1530  1 "$tmp_outpath/WEEKLYOPT/" &
  echo "$exec_bardata $date_ $FILE_PROD_opt_mid_w IST_915 IST_1530  1 $tmp_outpath/WEEKLYOPT/"
  taskset -c 29,30,31 $exec_bardata $date_ $FILE_PROD_opt_mid_w IST_915 IST_1530  1 "$tmp_outpath/WEEKLYOPT/" &
  sleep 5
  echo "Wait for complete"
  hist_running=`ps aux | grep  -i "nse_historical_bar_data_generator" | grep -v 'grep' | wc -l`
  echo "Status: $hist_running"

  while [[ $hist_running -gt 0 ]]; do
  	sleep 5;
  	hist_running=`ps aux | grep  -i "nse_historical_bar_data_generator" | grep -v 'grep' | wc -l`
 	echo "Status: $hist_running"
  done
  echo "Completed For Day $date_"
  cat $tmp_outpath/MONTHLYOPT/MIDCPNIFTY >> $OUTPUT_PATH/MONTHLYOPT/MIDCPNIFTY
  cat $tmp_outpath/MONTHLYOPT/FINNIFTY >> $OUTPUT_PATH/MONTHLYOPT/FINNIFTY
  echo "Monthly Done $date_"
  >$tmp_outpath/WEEKLYOPT_2/FINNIFTY
  >$tmp_outpath/WEEKLYOPT_2/MIDCPNIFTY
  $exec_fixbardata $tmp_outpath/MONTHLYOPT/MIDCPNIFTY $tmp_outpath/WEEKLYOPT_2/MIDCPNIFTY
  $exec_fixbardata $tmp_outpath/MONTHLYOPT/FINNIFTY $tmp_outpath/WEEKLYOPT_2/FINNIFTY
  echo "Fxing Weekly"
  >$tmp_outpath/WEEKLYOPT_2/MIDCPNIFTY_W
  >$tmp_outpath/WEEKLYOPT_2/FINNIFTY_W
  $exec_fixbardata $tmp_outpath/WEEKLYOPT/MIDCPNIFTY $tmp_outpath/WEEKLYOPT_2/MIDCPNIFTY_W
  $exec_fixbardata $tmp_outpath/WEEKLYOPT/FINNIFTY $tmp_outpath/WEEKLYOPT_2/FINNIFTY_W
  echo "SLEEP 5m"
  
  cat $tmp_outpath/WEEKLYOPT_2/MIDCPNIFTY_W >>$tmp_outpath/WEEKLYOPT_2/MIDCPNIFTY
  cat $tmp_outpath/WEEKLYOPT_2/FINNIFTY_W >>$tmp_outpath/WEEKLYOPT_2/FINNIFTY
	
  cat $tmp_outpath/WEEKLYOPT_2/MIDCPNIFTY >>$OUTPUT_PATH/WEEKLYOPT/MIDCPNIFTY
  cat $tmp_outpath/WEEKLYOPT_2/FINNIFTY >>$OUTPUT_PATH/WEEKLYOPT/FINNIFTY
  echo "Weekly Done $date_"
  date_=`date -d "$date_ +1 day" +"%Y%m%d"`
done

sort $OUTPUT_PATH/WEEKLYOPT/MIDCPNIFTY | uniq >/home/dvctrader/trash/temp_database
mv /home/dvctrader/trash/temp_database $OUTPUT_PATH/WEEKLYOPT/MIDCPNIFTY

sort $OUTPUT_PATH/WEEKLYOPT/FINNIFTY | uniq >/home/dvctrader/trash/temp_database
mv /home/dvctrader/trash/temp_database $OUTPUT_PATH/WEEKLYOPT/FINNIFTY

sort $OUTPUT_PATH/MONTHLYOPT/MIDCPNIFTY | uniq >/home/dvctrader/trash/temp_database
mv /home/dvctrader/trash/temp_database $OUTPUT_PATH/MONTHLYOPT/MIDCPNIFTY

sort $OUTPUT_PATH/MONTHLYOPT/FINNIFTY | uniq >/home/dvctrader/trash/temp_database
mv /home/dvctrader/trash/temp_database $OUTPUT_PATH/MONTHLYOPT/FINNIFTY

#taskset -c 23,24,25 /NAS4/raghu/BarData_Options/cvquant_install/basetrade/bin/nse_historical_bar_data_generator 20220117 /home/dvctrader/raghu/BarData_Options/FILES/banknifty_shortcode_options_monthly  IST_915 IST_1530  1 /home/dvctrader/raghu/BarData_Options/INDEX_BARDATA/MONTHLYOPT/
