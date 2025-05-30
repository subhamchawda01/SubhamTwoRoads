#!/bin/bash
USAGE="$0  YYYYMMDD";
#DEST_SERVER="192.168.132.11"
DEST_SERVER="192.168.132.12"
data_copy_info_file='/spare/local/data_copy_update.txt'
update_local_status='/spare/local/files/status_datacopy_and_sync.txt'
worker_sync_log='/tmp/datacopy_sync_logs_to_worker'

check_dest_alive(){
        while true;do
        ping -c1 -w 10 $DEST_SERVER
                if [ $? -ne 0 ] ; then
                        sleep 5m; echo "Retrying the connection"; continue;
                fi
        break;
        done
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
    echo "BSE holiday..., exiting";
    exit -1
fi

YYYY=${date:0:4}
MM=${date:4:2}
yyyy=${date:0:4}
mm=${date:4:2}
dd=${date:6:2}
remote_Dir="/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/"
local_Dir="/NAS1/data/BSELoggedData/BSE/${yyyy}/${mm}/${dd}/"
worker_Dir="/NAS5/data/BSELoggedData/BSE/${yyyy}/${mm}/${dd}/"
echo "$worker_Dir"
#echo "Copy Data from INDB11"
echo "Copy Data from INDB12"
while true; do
  rsync -asvz --progress $DEST_SERVER:$remote_Dir $local_Dir
  local_count=`ls $local_Dir| wc -l` ;
  [[ $local_count -lt 5200 ]] && { echo "LESS Count local: $local_count Sleeping"; sleep 1m; continue; }
  break
done

ssh dvctrader@54.90.155.232 "mkdir -p $worker_Dir"
echo "syncing Worker1"
while true; do
  rsync -avz --progress $local_Dir dvctrader@54.90.155.232:${worker_Dir}
  local_count=`ssh dvctrader@54.90.155.232 "ls ${worker_Dir}| wc -l"` ;
  [[ $local_count -lt 5200 ]] && { echo "Worker Count LESS Count local: $local_count Sleeping"; sleep 1m; continue; }
  break
done

# RUN EOD
echo "Running EOD"
/home/dvcinfra/important/datacopy_complete_bse.sh ${date}



ssh dvctrader@44.202.186.243 "mkdir -p $worker_Dir"
echo "syncing Worker2"
while true; do
  rsync -avz --progress $local_Dir dvctrader@44.202.186.243:${worker_Dir}
  local_count=`ssh dvctrader@44.202.186.243 "ls ${worker_Dir}| wc -l"` ;
  [[ $local_count -lt 5200 ]] && { echo "Worker Count LESS Count local: $local_count Sleeping"; sleep 1m; continue; }
  break
done

echo "Retry syncing Worker1"
while true; do
  rsync -avz --progress $local_Dir dvctrader@54.90.155.232:${worker_Dir}
  local_count=`ssh dvctrader@54.90.155.232 "ls ${worker_Dir}| wc -l"` ;
  [[ $local_count -lt 5200 ]] && { echo "Worker Count LESS Count local: $local_count Sleeping"; sleep 1m; continue; }
  break
done


echo "Sycning Second Local"
ssh 10.23.5.22 "mkdir -p $local_Dir"
while true; do
  rsync -avz --progress $local_Dir 10.23.5.22:${local_Dir}
  local_count=`ssh 10.23.5.22 "ls $local_Dir| wc -l"` ;
  [[ $local_count -lt 5200 ]] && { echo "Second Local LESS Count local: $local_count Sleeping"; sleep 1m; continue; }
  break
done


local_data_count=`ls -lrt ${local_Dir} | wc -l`
remote_data_count=`ssh $DEST_SERVER "cd ${remote_Dir};ls -lrt | wc -l"`
remote_data_count=$(( remote_data_count - 5 ))
worker_data_count=`ssh dvctrader@54.90.155.232 "cd ${worker_Dir};ls -lrt | wc -l"`;
worker_data_count2=`ssh dvctrader@54.90.155.232 "cd ${worker_Dir};ls -lrt | wc -l"`;

if [ ${remote_data_count} -gt ${local_data_count} ] || [ $local_data_count -ne ${worker_data_count} ];
then
        echo "" | mailx -s "FAILED : BSE DATA COPY : ${YYYYMMDD} INDB12=> ${remote_data_count} S62: ${local_data_count} SW1: ${worker_data_count} SW2: ${worker_data_count2}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in 
        exit
fi

echo "" | mailx -s "BSE HFT DATA COPY : ${YYYYMMDD} : INDB12: ${remote_data_count} S62: ${local_data_count} : SW1: ${worker_data_count} SW2: ${worker_data_count2}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in


#rsync -avz --progress /spare/local/MDSlogs/2022 10.23.5.66:/NAS1/data/BSELoggedData/BSE

#rsync -avz /run/media/dvcinfra/NSE_MTBT_2022_BAKCUP_NEW/data/BSELoggedData/BSE/2022/03 dvctrader@54.90.155.232:/NAS5/data/BSELoggedData/BSE/2022/
