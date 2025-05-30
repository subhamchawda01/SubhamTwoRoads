#!/bin/bash
USAGE="$0  YYYYMMDD";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit -1;
fi

date=$1;
YYYYMMDD=$1
if [ $1 == "TODAY" ]; then
        date=`date +"%Y%m%d"`;
fi
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    exit -1
fi

YYYY=${date:0:4}
MM=${date:4:2}
yyyy=${date:0:4}
mm=${date:4:2}
dd=${date:6:2}
today=$date
echo "DATE: $today"

mail_report=/tmp/data_copy_spot_currency_report.html;

today_currency_start_time=`date -d"$today" +"%s"`;
today_currency_end_time=$((today_currency_start_time+41340));
> /tmp/bardata_product_currency
for prod in EURINR USDINR EURUSD GBPINR GBPUSD JPYINR USDJPY; do ssh dvctrader@54.90.155.232 "grep $today_currency_end_time /spare/local/BarData/$prod"; done >> /tmp/bardata_product_currency;
currency_bardata_count=`cat /tmp/bardata_product_currency | wc -l`
echo "CurrencyBarDataCount: $currency_bardata_count  StartTime: $today_currency_start_time  EndTime: $today_currency_end_time"

today_spot_start_time=`date -d"$today" +"%s"`;
today_spot_end_time=$((today_spot_start_time+35940));
ssh dvctrader@54.90.155.232 "grep $today_spot_end_time /spare/local/BarData_SPOT/* " > /tmp/bardata_product_spot;
spot_bardata_count=`cat /tmp/bardata_product_spot | wc -l`
echo "SpotBarDataCount: $spot_bardata_count  StartTime: $today_spot_start_time  EndTime: $today_spot_end_time"


#Currnecy Bar Data
echo "<div>WORKER_CURRENCY_BARDATA_COUNT -> $currency_bardata_count</div>" > $mail_report ;

cat /tmp/bardata_product_currency | awk '{print $2}' | awk -F'_' '{print $1}' >/tmp/tmp_fileoutput_currency
mv /tmp/tmp_fileoutput_currency /tmp/bardata_product_currency
currency_products_file='/tmp/currency_bar_data_products'
echo -e "EURINR\nUSDINR\nEURUSD\nGBPINR\nGBPUSD\nJPYINR\nUSDJPY" > ${currency_products_file}
comm -3 <(sort /tmp/bardata_product_currency) <(sort ${currency_products_file}) >/tmp/diff_products_missing_barcount_currency
echo "<div style='margin:0 30px;'>" >>$mail_report ;
echo "<h3>Currency Bar Data Not Generated for:</h3>" >>$mail_report
echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Missing</th><th>Last Update</th></tr></thead><tbody>">> $mail_report
for line in `cat /tmp/diff_products_missing_barcount_currency`;
do
    line=${line//&/\\&}
    time_tt=`ssh dvctrader@54.90.155.232 "tail -1 /spare/local/BarData/$line"|awk '{print $1}'`
    echo $time_tt
    time_ttt=`date -d"@$time_tt"`
    echo $time_ttt
    echo -e "<tr><td style='border: 1px solid black' >$line</td><td style='border: 1px solid black' >$time_ttt</td></tr>\n" >> $mail_report ;
done
echo -e "</tbody></table>\n" >> $mail_report ;
echo "</div>" >> $mail_report ;
echo "</div> " >> $mail_report ;
echo "<br>" >> $mail_report;
echo "<br>" >> $mail_report;


#Spot Bar Data
echo "<div>WORKER_SPOT_BARDATA_COUNT -> $spot_bardata_count</div>" >> $mail_report ;

cat /tmp/bardata_product_spot | awk '{print $1}'| awk -F'/' '{print $5}' | awk -F':' '{print $1}' >/tmp/tmp_fileoutput_spot
mv /tmp/tmp_fileoutput_spot /tmp/bardata_product_spot
spot_products_file='/tmp/spot_bar_data_products'
scp dvctrader@54.90.155.232:/spare/local/files/NSE/spot_index_token.txt /spare/local/files/NSE/spot_index_token.txt
awk -F'[ _]' '{print $3}' /spare/local/files/NSE/spot_index_token.txt > ${spot_products_file}
comm -3 <(sort /tmp/bardata_product_spot) <(sort ${spot_products_file}) >/tmp/diff_products_missing_barcount_spot
echo "<div style='margin:0 30px;'>" >>$mail_report ;
echo "<h3>Spot Bar Data Not Generated for:</h3>" >>$mail_report
echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Missing</th><th>Last Update</th></tr></thead><tbody>">> $mail_report
for line in `cat /tmp/diff_products_missing_barcount_spot`;
do
    echo "$line"
    line=${line//&/\\&}
    time_tt=`ssh dvctrader@54.90.155.232 "tail -1 /spare/local/BarData_SPOT/$line" |awk '{print $1}'`
    if [ -z "$time_tt" ];then
      time_ttt=" "
    else
      time_ttt=`date -d"@$time_tt"`
    fi
    echo $time_tt
    echo $time_ttt
 echo -e "<tr><td style='border: 1px solid black' >$line</td><td style='border: 1px solid black' >$time_ttt</td></tr>\n" >> $mail_report ;
done
echo -e "</tbody></table>\n" >> $mail_report ;
echo "</div>" >> $mail_report ;
echo "</div> " >> $mail_report ;


(
  echo "To: subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in"
  echo "Subject: NSE BARDATA CHECK FOR SPOT AND CURRENCY ${today}"
  echo "Content-Type: text/html"
  echo
  cat $mail_report
  echo
) | /usr/sbin/sendmail -t
