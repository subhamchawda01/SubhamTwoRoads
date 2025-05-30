#!/bin/bash

today=`date +"%Y%m%d"`;

if [ $# -eq 1 ];
then
    today=$1;
fi

echo "Date: $today Data in Sync "
rsync -avz --progress --exclude="*_CE_*" --exclude="*_PE_*" /NAS1/data/NSELoggedData/NSE 44.202.186.243:/NAS1/data/NSELoggedData/
sleep 10
echo "Try Again"
rsync -avz --progress --exclude="*_CE_*" --exclude="*_PE_*" /NAS1/data/NSELoggedData/NSE 44.202.186.243:/NAS1/data/NSELoggedData/

echo "Assuming Data exist on WOrker_2 Now"
echo "StartEODJobs $today" >>/spare/local/files/eod_complete.txt
ssh dvctrader@44.202.186.243 "/home/dvctrader/EOD_SCRIPTS/eod_jobs_Only_Ratio.sh $today " >/tmp/logs_run_start_ratio_on_worker_new 2>&1 &
sleep 10;
echo "Running Bar DATA on Worker_1  Now"
/home/dvctrader/EOD_SCRIPTS/eod_jobs_Only_Bar.sh $today
echo "Bar DATA Completed"
EOD_Instance=`ssh dvctrader@44.202.186.243 "ps aux | egrep '/home/dvctrader/EOD_SCRIPTS/eod_jobs_Only_Ratio.sh|/home/dvctrader/stable_exec/scripts/calc_ratio.sh' | grep -v grep | wc -l"`
while [ $EOD_Instance -gt 0 ]; 
do
  echo "Ratio Running On worker2 dvctrader@44.202.186.243 Instance: $EOD_Instance"
  sleep 10;
  EOD_Instance=`ssh dvctrader@44.202.186.243 "ps aux | egrep '/home/dvctrader/EOD_SCRIPTS/eod_jobs_Only_Ratio.sh|/home/dvctrader/stable_exec/scripts/calc_ratio.sh' | grep -v grep | wc -l"`
done

rsync -avz --progress dvctrader@44.202.186.243:/spare/local/NseHftFiles/Ratio /spare/local/NseHftFiles
sleep 10
rsync -avz --progress dvctrader@44.202.186.243:/spare/local/NseHftFiles/Ratio /spare/local/NseHftFiles
sleep 10
rsync -avz --progress dvctrader@44.202.186.243:/spare/local/NseHftFiles/Ratio /spare/local/NseHftFiles
echo "FullEODJobs $today" >>/spare/local/files/eod_complete.txt

