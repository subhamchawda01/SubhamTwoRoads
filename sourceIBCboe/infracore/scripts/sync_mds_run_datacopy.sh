#!/bin/bash
USAGE="$0  YYYYMMDD";
DEST_SERVER="10.23.227.63"
data_copy_info_file='/spare/local/data_copy_update.txt'
update_local_status='/spare/local/files/status_datacopy_and_sync.txt'
worker_sync_log='/tmp/datacopy_sync_logs_to_worker'
rsync_13=-10
rsync_worker=-10
rsync_worker_2=-10
ps_count=0
>$worker_sync_log
get_expiry_date () {
  ONE=01;
  EXPIRY=$YYYYMMDD;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

check_dest_alive(){
	while true;do 
        ping -c1 -w 10 $DEST_SERVER
        	if [ $? -ne 0 ] ; then
			sleep 5m; echo "Retrying the connection"; continue;
        	fi
        break;
	done
}

start_syncind(){
	kill -0 $rsync_13
	if [ $? -ne 0 ]; then
		echo "No sync from IND13. Starting Sync";
		rsync -avz --progress --inplace --exclude="*_CE_*" --exclude="*_PE_*" $DEST_SERVER:$remote_Dir $local_Dir &
		rsync_13=$!
        else
           echo "Checking Count"
           new_count=`ls $local_Dir| egrep -v "_PE_|_CE_" | wc -l`
           if [ $new_count -eq $ps_count ]; then
                echo "Count is not Diff Killing and Starting Rsync $ps_count $new_count"
                kill $rsync_13
                rsync -avz --progress --inplace --exclude="*_CE_*" --exclude="*_PE_*" $DEST_SERVER:$remote_Dir $local_Dir &
                rsync_13=$!
           fi
           ps_count=$new_count    
	fi
	echo "Rsync Running IND13"
}

start_sync_worker(){
        kill -0 $rsync_worker
        if [ $? -ne 0 ]; then
                echo "No sync To Worker. Starting Sync";
		rsync -avz --progress --inplace --exclude="*_CE_*" --exclude="*_PE_*" $local_Dir dvctrader@54.90.155.232:${local_Dir} >>$worker_sync_log &
		rsync_worker=$!
        fi
	echo "Rsync Running Worker1"
        kill -0 $rsync_worker_2
        if [ $? -ne 0 ]; then
                echo "No sync To Worker. Starting Sync";
                rsync -avz --progress --inplace --exclude="*_CE_*" --exclude="*_PE_*" $local_Dir dvctrader@44.202.186.243:${local_Dir} >>$worker_sync_log &
                rsync_worker_2=$!
        fi
        echo "Rsync Running Worker"
}

check_entry_zero(){
  data_cash_comp=`ssh $DEST_SERVER "grep \"$date CASH 0\" $data_copy_info_file| wc -l"`
  if [[ $data_cash_comp -gt 0 ]];then
      echo ""| mailx -s "Problem with Data Convertion for Cash data" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
      exit -1
  fi
  data_fut_comp=`ssh $DEST_SERVER "grep \"$date FUT 0\" $data_copy_info_file| wc -l"`
  if [[ $data_fut_comp -gt 0 ]];then
      echo ""| mailx -s "Problem with Data Convertion for FUT data" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
      exit -1
  fi
}

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
remote_Dir="/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/"
local_Dir="/NAS1/data/NSELoggedData/NSE/${yyyy}/${mm}/${dd}/"
mkdir -p $local_Dir

ssh dvctrader@54.90.155.232 "mkdir -p $local_Dir"
ssh dvctrader@44.202.186.243 "mkdir -p $local_Dir"
while true; do
	check_dest_alive ;
	start_syncind;
	sleep 2m;
	start_sync_worker;
	sleep 2m;
	data_fut_comp=`ssh $DEST_SERVER "grep \"$date FUT\" $data_copy_info_file| wc -l"`
        echo "fut -$data_fut_comp"
	[[ $data_fut_comp -eq 0 ]] && continue;
	data_cash_comp=`ssh $DEST_SERVER "grep \"$date CASH\" $data_copy_info_file| wc -l"`
        echo "cash -$data_cash_comp"
	[[ $data_cash_comp -eq 0 ]] && continue;
        check_entry_zero;
	remote_count=`ssh $DEST_SERVER "ls $remote_Dir | egrep -v \"_PE_|_CE_\" | wc -l"` ;
	local_count=`ls $local_Dir | egrep -v "_PE_|_CE_" | wc -l` ;
        echo "Count $remote_count $local_count"
	[[ $remote_count -ne $local_count ]] && continue;
	break;
done

echo "checking data sync to the worker"
#sync data to worker

kill -0 $rsync_worker
while true; do
	rsync -avz --progress --exclude="*_CE_*" --exclude="*_PE_*" $local_Dir dvctrader@54.90.155.232:${local_Dir}
	[ $? -ne 0 ] &&  { echo "Retrying..."; sleep 2m; continue; }
	break;
done

kill -0 $rsync_worker_2
while true; do
        rsync -avz --progress --exclude="*_CE_*" --exclude="*_PE_*" $local_Dir dvctrader@44.202.186.243:${local_Dir}
        [ $? -ne 0 ] &&  { echo "Retrying..."; sleep 2m; continue; }
        break;
done

echo "Running EOD jobs to the worker"
#Run EOD jobs
echo "$date DATACOPYWORKER_SYNC_FUT_CASH" >>$update_local_status
/home/dvcinfra/important/datacopy_complete.sh ${date}
echo "$date DATACOPYWORKER_SCRIPT" >>$update_local_status

get_expiry_date;
echo "Expiry : $EXPIRY";
EXPIRY=`/home/pengine/prod/live_execs/update_date $EXPIRY P W`
is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`
while [ $is_holiday_ = "1" ];
do
    EXPIRY=`/home/pengine/prod/live_execs/update_date $EXPIRY P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`
done
echo "day before expiry: $EXPIRY"
if [[ $EXPIRY == $date ]]; then
	echo "" | mailx -s "REMINDER: Please adjust Bardata for today $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nishit.bhandari@tworoads.co.in  
fi
echo "Adjust FINNIFTY & MIDCAP DATA ON "
tues_exp=`/home/pengine/prod//live_execs/get_expiry_from_shortcode NSE_FINNIFTY_FUT0 $date`
tues_exp=`/home/pengine/prod/live_execs/update_date $tues_exp P W`
is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $tues_exp T`
while [ $is_holiday_ = "1" ];
do
    tues_exp=`/home/pengine/prod/live_execs/update_date $tues_exp P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $tues_exp T`
done

echo "Day before Tuesday expiry $tues_exp "
if [[ $tues_exp == $date ]]; then
        echo "" | mailx -s "REMINDER: Please adjust MIDCAP AND FINNIFTY Bardata for today $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nishit.bhandari@tworoads.co.in
fi




#Run DB JOBS

/home/pengine/prod/live_scripts/db_jobs_complete.sh ${date} >/tmp/db_logs_fronm_worker_run_check 2>&1

