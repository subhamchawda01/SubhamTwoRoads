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

/home/dvcinfra/important/check_datasync_bse.sh ${today} > ~/trash/datasync_bse.txt 2>&1
status=$?

#check exit status of the script, fire eod jobs only if the data sync is fine
if [ $status -ne 0 ];
then
  exit;
fi

ssh dvctrader@44.202.186.243 "/home/dvctrader/EOD_SCRIPTS/eod_jobs_bse.sh $today";
sleep 10;

EOD_Instance=`ssh dvctrader@44.202.186.243 'ps aux | grep /home/dvctrader/EOD_SCRIPTS/eod_jobs_bse.sh | grep -v grep | wc -l'`
while [ $EOD_Instance -gt 0 ]; 
do
  sleep 2;
  EOD_Instance=`ssh dvctrader@44.202.186.243 'ps aux | grep /home/dvctrader/EOD_SCRIPTS/eod_jobs_bse.sh | grep -v grep | wc -l'`
done

echo "$date DATACOPYWORKER_EOD_COMPLETE" >>$update_local_status

echo "Uploading"
ssh dvctrader@10.23.5.26 '/home/dvctrader/important/ratio_upload_bse.sh' > ~/trash/sync_bse.txt 
echo "Uploaded"

start_ratio_count_worker=`ssh dvctrader@44.202.186.243 "grep $today /spare/local/BseHftFiles/Ratio/StartRatio/BSE_*  | wc -l"`;
end_ratio_count_worker=`ssh dvctrader@44.202.186.243 "grep $today /spare/local/BseHftFiles/Ratio/EndRatio/BSE_*  | wc -l"`;
echo "CASH Worker"
files_count=`ssh dvctrader@44.202.186.243 "ls /NAS1/data/BSELoggedData/BSE/$yyyy/$mm/$dd/ | wc -l"` ;
echo "Files Count: $files_count"

echo "<div>DATA_FILES_COUNT -> $files_count</div>" >> $mailfile;
echo "==============================================================" >> $mailfile;

echo "<div>START_RATIO_WORKER_COUNT -> $start_ratio_count_worker</div>" >> $mailfile;
echo "<div>END_RATIO_WORKER_COUNT -> $end_ratio_count_worker</div>" >> $mailfile;
echo "Server"
for i in 11 12 ; do  

SERVER=`echo "192.168.132.$i"`;
start_count=`ssh dvctrader@192.168.132.$i "grep $today /spare/local/BseHftFiles/Ratio/StartRatio/BSE_*  | wc -l"`;
end_count=`ssh dvctrader@192.168.132.$i "grep $today /spare/local/BseHftFiles/Ratio/EndRatio/BSE_*  | wc -l"`;

echo "<div>RATIO_COUNT_$SERVER -> $start_count : $end_count</div>" >> $mailfile;

done

(
  echo "To: raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, rahul.yadav@tworoads-trading.co.in, naman.jain@tworoads-trading.co.in, tarun.joshi@tworoads-trading.co.in"

#  echo "To: raghunandan.sharma@tworoads-trading.co.in"
  echo "Subject: BSE DATACOPY FOR ${today} COMPLETED FOR CASH AND FUT AND EOD JOBS COMPLETED"
  echo "Content-Type: text/html"
  echo
  cat $mail_report
  echo
) | /usr/sbin/sendmail -t

