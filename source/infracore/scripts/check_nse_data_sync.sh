#!/bin/bash

GetPreviousWorkingDay() {
  prev_day=`/home/pengine/prod/live_execs/update_date $today P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
  while [ $is_holiday_ = "1" ];
  do
    prev_day=`/home/pengine/prod/live_execs/update_date $prev_day P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
  done
}



today=`date +"%Y%m%d"`
GetPreviousWorkingDay
yyyy=${prev_day:0:4}
mm=${prev_day:4:2}
dd=${prev_day:6:2}
mailfile="/tmp/mailfile"
>${mailfile}


remote_data_dir='/spare/local/MDSlogs/'${yyyy}'/'${mm}'/'${dd}'/'
local_data_dir='/NAS1/data/NSELoggedData/NSE/'${yyyy}'/'${mm}'/'${dd}'/'

local_data_count=`ls -lrt ${local_data_dir} | wc -l`
remote_data_count=`ssh 10.23.227.63 "cd ${remote_data_dir};ls -lrt | wc -l"`
echo $local_data_count : $remote_data_count
attempts=0
while [ ${remote_data_count} -ne ${local_data_count} ] && [ $attempts -ne 8 ];
do
  echo rsync -avz --bwlimit=1024 --progress 10.23.227.63:${remote_data_dir} ${local_data_dir} 
  rsync -avz --bwlimit=1024 --progress 10.23.227.63:${remote_data_dir} ${local_data_dir} >/dev/null 2>/dev/null 
  local_data_count=`ls -lrt ${local_data_dir} | wc -l`
  attempts=$((attempts+1))
done

#we are done with syncing to local machine.. start syncing  to worker
worker_data_count=`ssh dvcinfra@3.89.148.73 "cd ${local_data_dir};ls -lrt | wc -l"`;
echo $worker_data_count
attempts=0;
while [ ${worker_data_count} -ne ${local_data_count} ] && [ $attempts -ne 8 ];
do
  echo rsync -avz --progress ${local_data_dir} dvcinfra@3.89.148.73:${local_data_dir} 
  rsync -avz --progress ${local_data_dir} dvctrader@3.89.148.73:${local_data_dir} >/dev/null 2>/dev/null 
  worker_data_count=`ssh dvcinfra@3.89.148.73 "cd ${local_data_dir};ls -lrt | wc -l"`;
  attempts=$((attempts+1))
done

if [ ${remote_data_count} -ne ${local_data_count} ] || [ $local_data_count -ne ${worker_data_count} ];
then
	echo "" | mailx -s "FAILED : NSE DATA COPY : ${prev_day} => ${remote_data_count} : ${local_data_count} : ${worker_data_count}" -r $HOSTNAME sanjeev.kumar@tworoads-trading.co.in  hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
	exit
fi

echo "FILES COUNT: ${prev_day} : ${remote_data_count} : ${local_data_count} : ${remote_data_count}" >> /home/dvcinfra/important/summary_data_sync

echo "" | mailx -s "HFT DATA COPY : ${prev_day} : ${remote_data_count} : ${local_data_count} : ${remote_data_count}" -r sanjeev.kumar@tworoads-trading.co.in  sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
