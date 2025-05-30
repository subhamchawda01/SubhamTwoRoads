#!/bin/bash
DEST_SERVER="10.23.227.63"

USAGE="$0  YYYYMMDD";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

if [ $1 == "TODAY" ];
then
  date=`date +"%Y%m%d"`
else
  date=$1
fi



is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    exit
fi

yyyy=${date:0:4}
mm=${date:4:2}
dd=${date:6:2}
remote_Dir="/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/"
local_Dir="/NAS1/data/NSELoggedData/NSE/${yyyy}/${mm}/${dd}/"
mkdir -p $local_Dir

'''
while true; do
    echo "Trying to Sync from Local server 67"    
    rsync -avz --progress 10.23.5.67:/NAS1/data/NSELoggedData/NSE /NAS1/data/NSELoggedData/
    status=$?
    [ $status -ne 0 ] && { echo "Retrying..."; sleep 5m; continue;  }
    local_count=`ls $local_Dir| wc -l` ;
    [[ $local_count -lt 2000 ]] && { echo "Count Less than 2000 $local_count Sleeping"; sleep 5m; continue; }
    break
done
'''

while true; do
    echo "Trying to Sync DATA from ind13"
    rsync -avz --progress 10.23.227.63:/spare/local/MDSlogs/${date:0:4}/${date:4:2}/${date:6:2} /NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}
    status1=$?
    echo "sycning to Working"
    rsync  -avz --progress /NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}/${date:6:2} dvctrader@52.90.0.239:/NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}
    status2=$?
#    [ $status1 -ne 0 ] && { echo "Sync Failed from IND13 Retrying..."; sleep 5m; continue;  } // getting chmod error 
    [ $status2 -ne 0 ] && { echo "Sync Failed to Worker Retrying..."; sleep 1m; continue;  }
    local_count=`ls $local_Dir| wc -l` ;
    remote_data_count=`ssh 10.23.227.63 "cd ${remote_Dir};ls| wc -l"`
   
    [[ $local_count -ne $remote_data_count ]] && { echo "Count remote: $remote_data_count  Not Equal local: $local_count Sleeping"; sleep 1m; continue; }
    [[ $local_count -lt 26000 ]] && { echo "LESS Count local: $local_count Sleeping"; sleep 1m; continue; }
    break
done


local_data_count=`ls -lrt ${local_Dir} | wc -l`
remote_data_count=`ssh 10.23.227.63 "cd ${remote_Dir};ls -lrt | wc -l"`
worker_data_count=`ssh dvctrader@52.90.0.239 "cd ${local_Dir};ls -lrt | wc -l"`;

echo "FILES COUNT: ${date} : ${remote_data_count} : ${local_data_count} : ${remote_data_count}"

if [ ${remote_data_count} -ne ${local_data_count} ] || [ $local_data_count -ne ${worker_data_count} ];
then
        echo "" | mailx -s "FAILED : NSE DATA COPY : ${prev_day} => ${remote_data_count} : ${local_data_count} : ${worker_data_count}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, nishit.bhandari@tworoads.co.in, raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, rahul.yadav@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in, arpit.agarwal@tworoads-trading.co.in 
        exit
fi


echo "" | mailx -s "HFT DATA COPY : ${prev_day} : ${remote_data_count} : ${local_data_count} : ${remote_data_count}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, nishit.bhandari@tworoads.co.in, subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in, arpit.agarwal@tworoads-trading.co.in 

