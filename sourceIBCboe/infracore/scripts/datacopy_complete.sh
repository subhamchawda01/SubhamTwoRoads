#!/bin/bash

if [ $# -ne 1 ] ; then
  echo "Called As : YYYYMMDD" ;
  exit ;
fi

today=$1;
yyyy=${today:0:4}
mm=${today:4:2}
dd=${today:6:2}
mailfile=/tmp/data_copy_ratio_report.html;
update_local_status='/spare/local/files/status_datacopy_and_sync.txt';
mail_report=$mailfile
>$mailfile;

/home/dvcinfra/important/check_datasync.sh ${today} > ~/trash/datasync.txt 2>&1
status=$?

#check exit status of the script, fire eod jobs only if the data sync is fine
if [ $status -ne 0 ];
then
  exit;
fi

ssh dvctrader@54.90.155.232 "/home/dvctrader/EOD_SCRIPTS/eod_jobs.sh $today";
sleep 10;

EOD_Instance=`ssh dvctrader@54.90.155.232 'ps aux | grep /home/dvctrader/EOD_SCRIPTS/eod_jobs.sh | grep -v grep | wc -l'`
while [ $EOD_Instance -gt 0 ]; 
do
  sleep 2;
  EOD_Instance=`ssh dvctrader@54.90.155.232 'ps aux | grep /home/dvctrader/EOD_SCRIPTS/eod_jobs.sh | grep -v grep | wc -l'`
done

echo "$date DATACOPYWORKER_EOD_COMPLETE" >>$update_local_status

ssh dvctrader@10.23.5.26 '/home/dvctrader/important/ratio_upload.sh' > ~/trash/sync.txt 
start_ratio_count_worker=`ssh dvctrader@54.90.155.232 "grep $today /spare/local/NseHftFiles/Ratio/StartRatio/NSE_*_FUT0  | wc -l"`;
end_ratio_count_worker=`ssh dvctrader@54.90.155.232 "grep $today /spare/local/NseHftFiles/Ratio/EndRatio/NSE_*_FUT0  | wc -l"`;
files_count=`ssh 54.90.155.232 "ls /NAS1/data/NSELoggedData/NSE/$yyyy/$mm/$dd/ | wc -l"` ;

echo "<div>DATA_FILES_COUNT -> $files_count</div>" >> $mailfile;
echo "==============================================================" >> $mailfile;
echo "<div>START_RATIO_WORKER_COUNT -> $start_ratio_count_worker</div>" >> $mailfile;
echo "<div>END_RATIO_WORKER_COUNT -> $end_ratio_count_worker</div>" >> $mailfile;
echo "Server"
for i in 65 69 81 82 83 84 71 72; do  

SERVER=`echo "10.23.227.$i"`;
start_count=`ssh dvctrader@10.23.227.$i "grep $today /spare/local/NseHftFiles/Ratio/StartRatio/NSE_*_FUT0  | wc -l"`;
end_count=`ssh dvctrader@10.23.227.$i "grep $today /spare/local/NseHftFiles/Ratio/EndRatio/NSE_*_FUT0  | wc -l"`;

echo "<div>RATIO_COUNT_$SERVER -> $start_count : $end_count</div>" >> $mailfile;

done
today_start_time=`date -d"$today" +"%s"`;
today_end_time=$((today_start_time+35940));
ind19_bardata_count=`ssh dvctrader@10.23.227.69 "grep $today_end_time /spare/local/BarData/* | wc -l"` ;

echo "==============================================================" >> $mailfile;

echo "<div>IND19_BARDATA_COUNT -> $ind19_bardata_count</div>" >> $mailfile ;
#Bar Data
yesterday=$today
echo "yest $yesterday"
today_start_time=`date -d"$yesterday" +"%s"`
today_end_time=$((today_start_time+35940));
echo "$today_end_time"
ssh 10.23.227.69 "grep $today_end_time /spare/local/BarData/*" >/tmp/bardata_product_ind19
cat /tmp/bardata_product_ind19 | awk '{print $1}'| awk -F'/' '{print $5}' | awk -F':' '{print $1}' >/tmp/tmp_fileoutput
mv /tmp/tmp_fileoutput /tmp/bardata_product_ind19
bar_data_count=`cat /tmp/bardata_product_ind19 | wc -l`
echo -e "Bardata: $bar_data_count\n"
products_file='/tmp/bar_data_products'
>${products_file}
tail -n +2 "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${today}.csv" | awk  -F "," '{print $2}' | awk '{print $1}'  > ${products_file}
comm -3 <(sort /tmp/bardata_product_ind19) <(sort ${products_file}) >/tmp/diff_products_missing_barcount
echo "<div style='margin:0 30px;'>" >>$mail_report ;
echo "<h3>Bar Data Not Generated for:</h3>" >>$mail_report
echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Missing</th><th>Last Update</th></tr></thead><tbody>">> $mail_report
for line in `cat /tmp/diff_products_missing_barcount`;
do
    line=${line//&/\\&}
    time_tt=`ssh 10.23.227.69 "tail -1 /spare/local/BarData/$line"|awk '{print $1}'`
    echo $time_tt
    time_ttt=`date -d"@$time_tt"`
    echo $time_ttt
    echo -e "<tr><td style='border: 1px solid black' >$line</td><td style='border: 1px solid black' >$time_ttt</td></tr>\n" >> $mail_report ;
done
echo -e "</tbody></table>\n" >> $mail_report ;
echo "</div>" >> $mail_report ;
echo "</div>" >> $mail_report ;
shortcode_ratio="/tmp/shortcode_ratio_lotfile"
awk -F"," '/,/{gsub(/ /, "", $2); if (NR!=1) {if ( $2 !~ /NIFTY/ ) { print "NSE_"$2"_FUT1_NSE_"$2"_FUT0\nNSE_"$2"_NSE_"$2"_FUT0"} else {print "NSE_"$2"_FUT1_NSE_"$2"_FUT0"} } }' /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${today}.csv | sort > $shortcode_ratio
shortcode_server="/tmp/temp2PositionBar"
echo "<div style='margin:0 30px;'>" >>$mail_report ;
echo "<h3>Missing Start ratio Symbol:</h3>" >>$mail_report
echo "<table id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Missing</th></tr></thead><tbody>">> $mail_report
echo `ssh dvctrader@10.23.227.82 "grep -l $today /spare/local/NseHftFiles/Ratio/StartRatio/NSE_*FUT0|cut -d '/' -f 7"`|tr " " "\n" > $shortcode_server
sort $shortcode_server >/tmp/start_ratio_symbol_s
startrc=$(wc -l <"/tmp/start_ratio_symbol_s");
echo "serc "$startrc
comm -3 /tmp/start_ratio_symbol_s $shortcode_ratio >$shortcode_server
for line in `cat $shortcode_server`;
do
    echo -e "<tr><td style='border: 1px solid black' >$line</td></tr>\n" >> $mail_report ;
done
echo -e "</tbody></table>\n" >> $mail_report ;
echo "</div>" >> $mail_report ;

echo "<div style='margin:0 30px;'>" >>$mail_report ;
echo "<h3>Missing End ratio Symbol:</h3>" >>$mail_report
echo "<table id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Missing</th></tr></thead><tbody>">> $mail_report
echo `ssh dvctrader@10.23.227.82 "grep -l $today /spare/local/NseHftFiles/Ratio/EndRatio/NSE_*FUT0|cut -d '/' -f 7"`|tr " " "\n" > $shortcode_server
sort $shortcode_server >/tmp/end_ratio_symbol_s
endrc=$(wc -l <"/tmp/end_ratio_symbol_s");
echo "endrc "$endrc
comm -3  /tmp/end_ratio_symbol_s $shortcode_ratio >$shortcode_server
for line in `cat $shortcode_server`;
do
    echo -e "<tr><td style='border: 1px solid black' >$line</td></tr>\n" >> $mail_report ;
done
echo -e "</tbody></table>\n" >> $mail_report ;
echo "</div>" >> $mail_report ;

echo "</div>" >> $mail_report ;

(
  echo "To: raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, rahul.yadav@tworoads-trading.co.in"
#  echo "To: raghunandan.sharma@tworoads-trading.co.in"
  echo "Subject: NSE DATACOPY FOR ${today} COMPLETED FOR CASH AND FUT AND EOD JOBS COMPLETED"
  echo "Content-Type: text/html"
  echo
  cat $mail_report
  echo
) | /usr/sbin/sendmail -t

#cat $mailfile | /bin/mail -s "NSE DATACOPY FOR ${today} COMPLETED FOR CASH AND FUT AND EOD JOBS COMPLETED" -r $HOSTNAME sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in
