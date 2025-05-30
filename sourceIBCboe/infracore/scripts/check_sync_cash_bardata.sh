#!/bin/bash
ind18=dvctrader@10.23.227.83
BAR_DATA_DESTINATION_DIR="/NAS1/data/NSEBarData/CASH_BarData/"
tmp_product_file="/tmp/product_file_added_bardata_generated"

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}



declare -A server_to_ip_map
mail_file=/tmp/cash_bardata_report_sync.log
>$mail_file

server_to_ip_map=( ["IND16"]="10.23.227.81" \
                   ["IND17"]="10.23.227.82" \
                   ["IND18"]="10.23.227.83" \
                   ["IND24"]="10.23.227.74" \
                   ["IND23"]="10.23.227.72" )

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

today_=$1
YYYYMMDD=$1
GetPreviousWorkingDay
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4}
ftp_dir="/NAS1/data/NSEFTPFiles/$YYYY/$MM/$DD/"
lot_file="/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${YYYYMMDD}.csv"
mkt_data="/NAS1/data/NSELoggedData/NSE/$YYYY/$MM/$DD/"

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    exit
fi
today_start_time=`date -d"$YYYYMMDD" +"%s"`;
today_end_time=$((today_start_time+35940));
echo "END TIME: $today_end_time"
local_count=`tail /NAS1/data/NSEBarData/CASH_BarData/* | grep $today_end_time  | wc -l`
tail /NAS1/data/NSEBarData/CASH_BarData/* | grep $today_end_time >$tmp_product_file

echo '<!DOCTYPE html> <html><body>' >>$mail_file
echo '<div style="display:flex;justify-content:space-around;">' >>$mail_file
echo "<div style='margin:0 30px;'>" >>$mail_file
echo "<h3>Start Ratio Alert: </h3>">>$mail_file
echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>SERVER</th><th>Count</th></tr></thead><tbody>">> $mail_file
echo "LOCAL(5.66) :      $local_count"
echo -e "<tr><td>LOCAL(5.66) :</td><td>$local_count</td></tr>\n" >> $mail_file ;

for server in "${!server_to_ip_map[@]}";
do
  server_count=`ssh ${server_to_ip_map[$server]} "tail /spare/local/CASH_BarData/* | grep $today_end_time  | wc -l"`;
  echo "$server :              $server_count"
  echo -e "<tr><td>$server</td><td>$server_count</td></tr>\n" >> $mail_file ;
done
echo "</tbody></table></div>" >> $mail_file ;
echo "<h3>Missing BarData: </h3>">>$mail_file
echo "<table border='1' id='myTable' class='table table-striped' style='border-collapse: collapse'<thead><tr><th>Symbol</th><th>Last Update</th><th>EQ</th><th>FO</th><th>MKT_UDPATE</th></tr></thead><tbody>" >> $mail_file
ssh $ind18 "cat /home/dvctrader/ATHENA/run.sh | grep -v \"^#\"|grep MIDTERM |grep LIVE | cut -d' ' -f2" >/tmp/live_file_path_midterm.txt
tmp_pos_file="/tmp/pos_file_generated_ind18"
touch $tmp_pos_file
for pos_file in `cat /tmp/live_file_path_midterm.txt`; do
  scp $ind18:$pos_file $tmp_pos_file
  sed 1,2d $tmp_pos_file | cut -d' ' -f3 | cut -d'_' -f2 | sort |uniq >/tmp/short_code_data_generated_file_$today_
  for product in `cat /tmp/short_code_data_generated_file_$today_`; do
      if ! grep -q "${product}" $tmp_product_file
        then
          time_tt=`tail -1 /NAS1/data/NSEBarData/CASH_BarData/$product |awk '{print $1}'`
          echo $time_tt
          time_ttt=`date -d"@$time_tt"`
          echo $time_ttt
          is_eq=`grep "|$product|" $ftp_dir/security | grep EQ | wc -l`
          is_fo=`grep -w ",$product" $lot_file | wc -l`
          mkt_file="${mkt_data}/NSE_${product}_$YYYYMMDD.gz"
          mkt_update=`ls $mkt_file | wc -l`
          echo "FILE: $mkt_file";

          echo -e "<tr><td>${product}</td><td>$time_ttt</td><td>$is_eq</td><td>$is_fo</td><td>$mkt_update</td></tr>\n"  >> $mail_file
        fi      
  done
done

echo "</tbody></table></div></div></body></html>" >> $mail_file

  (
# echo "To: raghunandan.sharma@tworoads-trading.co.in"
 echo "To: raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, nseall@tworoads.co.in"
 echo "Subject: ***************CASH BARDATA $YYYYMMDD SYNC CHECK***************"
 echo "Content-Type: text/html"
 echo
 cat $mail_file
 echo
 ) | /usr/sbin/sendmail -t


