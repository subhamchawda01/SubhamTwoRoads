#!/bin/bash

today=`date +"%Y%m%d"`;
yyyy=${today:0:4}
mm=${today:4:2}
dd=${today:6:2}

is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday_ = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi


update_local_status='/spare/local/files/status_datacopy_and_sync.txt';
echo "RUNNING: /home/pengine/prod/live_scripts/sync_mds_run_datacopy.sh $today"
/home/pengine/prod/live_scripts/sync_mds_run_datacopy.sh $today


status=$?

#check exit status of the script, fire eod jobs only if the data sync is fine
if [ $status -ne 0 ];
then
	echo "NOT RUNNING sync_mds_run_datacopy as Status is $status"
  echo "" | mailx -s "NOT RUNNING sync_mds_run_datacopy as Status is $status" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
  exit;
fi
# DOUBLE CHECK

exist_update=`grep "$date" $update_local_status | grep "DATACOPYWORKER_EOD_COMPLETE" | wc -l`
echo "$exist_update"
if [[ $exist_update -lt 1 ]]; then
	echo "No Entry in $update_local_status ,Not Running Option sync"
	echo "" | mailx -s "No Entry in $update_local_status ,Not Running Option sync" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
	exit;
fi

echo "RUNNING SyncOptionsDay.sh FOR THE DAY $today"
/home/pengine/prod/live_scripts/SyncOptionsDay.sh $today

