#!/bin/bash

USAGE="$0    EXCHANGE(IND13)    CURRENT_LOCATION(5.67)  YYYYMMDD";
DEST_SERVER="10.23.227.63"
WORKER_SERVER="54.90.155.232"
USER="dvcinfra"
WORKER_USER="dvctrader"
Local_Dir="/NAS1/data/ORSData"
Remote_Dir="/spare/local/ORSBCAST"
Worker_Dir="/NAS4/data/ORSData/"

send_error(){
                  echo "" | mailx -s "ORS REPLY BACKUP WORKER ISSUE" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
                                    exit ;
}

if [ $# -ne 3 ] ;
then
    echo $USAGE
    exit;
fi
EXCHANGE=$1;
CURRENT_LOCATION=$2;
YYYYMMDD=$3;
[ $YYYYMMDD = "TODAY" ] && YYYYMMDD=$(date "+%Y%m%d");
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4}
echo "Syncing for Date: $YYYYMMDD"
Mail_File="/tmp/ors_worker_backup_mail_${YYYYMMDD}"
>${Mail_File}

date=$YYYYMMDD
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

#Convevrted

ssh ${WORKER_USER}@${WORKER_SERVER} "mkdir -p $Worker_Dir/$EXCHANGE/$YYYY/$MM/"
local_count=`ls "$Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD/" | wc -l`
worker_count=`ssh ${WORKER_USER}@${WORKER_SERVER} "ls $Worker_Dir/$EXCHANGE/$YYYY/$MM/$DD | wc -l"`
echo "Worker_Count: $worker_count"
echo "Local Count: $local_count"
i=0;
while [[ $local_count -ne $worker_count ]] ; do
  i=$((i+1))
  [[ $i > 4 ]] && { send_error; break; }
  echo "Syncing Converted ORS_REPLY Files..."
  rsync -ravz "$Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD" "${WORKER_USER}@$WORKER_SERVER:$Worker_Dir/$EXCHANGE/$YYYY/$MM/" 
#  echo "rsync -ravz $Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD ${WORKER_USER}@$WORKER_SERVER:$Worker_Dir/$EXCHANGE/$YYYY/$MM/" 
  [[ $? != 0 ]] && sleep 10m;
  worker_count=`ssh ${WORKER_USER}@${WORKER_SERVER} "ls $Worker_Dir/$EXCHANGE/$YYYY/$MM/$DD | wc -l"`
done
echo "WORKER CONVERTED ORS_REPLY SYNC DONE"

local_count=`ls "$Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD/" | wc -l`
worker_count=`ssh ${WORKER_USER}@${WORKER_SERVER} "ls $Worker_Dir/$EXCHANGE/$YYYY/$MM/$DD | wc -l"`
echo -e "CONVERTED ORS_REPLY COUNT:\n\n5.67: ${local_count}\nWorker: ${worker_count}\n" >> ${Mail_File}


#Q19
#IND13 to 5.67
#rsync -avz --progress 10.23.227.63:/spare/local/ORSBCAST_MULTISHM/NSE/*${today}* /NAS1/data/ORSData/ORSBCAST_MULTISHM/$yyyy/$mm/$dd
#DIRECTORIES
Q19_Local_Dir="/NAS1/data/ORSData/ORSBCAST_MULTISHM/"
Q19_Remote_Dir="/spare/local/ORSBCAST_MULTISHM/"
Q19_Worker_Dir="/NAS4/data/ORSData/ORSBCAST_MULTISHM/"

#SERVER IP'S
Q19_DEST_SERVER="10.23.227.63"
Q19_WORKER_SERVER="54.90.155.232"

q19_local_count=`ls "$Q19_Local_Dir/$YYYY/$MM/$DD/" | wc -l`
q19_remote_count=`ssh $Q19_DEST_SERVER "ls $Q19_Remote_Dir/$EXCHANGE/*$YYYYMMDD* | wc -l"`
echo "Q19_Remote_Count: $q19_remote_count"
echo "Q19_Local Count: $q19_local_count"
i=0;
mkdir -p "$Q19_Local_Dir/$YYYY/$MM/$DD/"
while [[ $q19_local_count -ne $q19_remote_count ]] ; do
  i=$((i+1))
  [[ $i > 4 ]] && { send_error; break; }
  echo "Syncing Local Q19 Data Files..."
  rsync -ravz "$Q19_DEST_SERVER:$Q19_Remote_Dir/$EXCHANGE/*$YYYYMMDD*" "$Q19_Local_Dir/$YYYY/$MM/$DD/"
#  echo "rsync -ravz $Q19_DEST_SERVER:$Q19_Remote_Dir/$EXCHANGE/*$YYYYMMDD* $Q19_Local_Dir/$YYYY/$MM/$DD/"
  [[ $? != 0 ]] && sleep 10m;
  q19_local_count=`ls "$Q19_Local_Dir/$YYYY/$MM/$DD/" | wc -l`
done
echo "LOCAL Q19 SYNC DONE"

#5.67 to worker
ssh ${WORKER_USER}@${Q19_WORKER_SERVER} "mkdir -p $Q19_Worker_Dir/$YYYY/$MM/$DD/"
#echo "ssh ${WORKER_USER}@${Q19_WORKER_SERVER} mkdir -p $Q19_Worker_Dir/$YYYY/$MM/$DD/"
q19_local_count=`ls "$Q19_Local_Dir/$YYYY/$MM/$DD/" | wc -l`
q19_worker_count=`ssh ${WORKER_USER}@$Q19_WORKER_SERVER "ls $Q19_Worker_Dir/$YYYY/$MM/$DD/ | wc -l"`
echo "Q19_Worker_Count: $q19_worker_count"
echo "Local Count: $q19_local_count"
i=0;
while [[ $q19_local_count -ne $q19_worker_count ]] ; do
  i=$((i+1))
  [[ $i > 4 ]] && { send_error; break; }
  echo "Syncing Q19 Worker Data Files..."
  rsync -ravz "$Q19_Local_Dir/$YYYY/$MM/$DD" "${WORKER_USER}@${Q19_WORKER_SERVER}:$Q19_Worker_Dir/$YYYY/$MM/" 
  echo "rsync -ravz $Q19_Local_Dir/$YYYY/$MM/$DD ${WORKER_USER}@${Q19_WORKER_SERVER}:$Q19_Worker_Dir/$YYYY/$MM/" 
  [[ $? != 0 ]] && sleep 10m;
  q19_worker_count=`ssh ${WORKER_USER}@${Q19_WORKER_SERVER} "ls $Q19_Worker_Dir/$YYYY/$MM/$DD/ | wc -l"`
done

q19_local_count=`ls "$Q19_Local_Dir/$YYYY/$MM/$DD/" | wc -l`
q19_remote_count=`ssh $Q19_DEST_SERVER "ls $Q19_Remote_Dir/$EXCHANGE/*$YYYYMMDD* | wc -l"`
q19_worker_count=`ssh ${WORKER_USER}@${Q19_WORKER_SERVER} "ls $Q19_Worker_Dir/$YYYY/$MM/$DD/ | wc -l"`
echo -e "\nQ19 ORS_REPLY COUNT:\n\nIND13: ${q19_remote_count}\n5.67: ${q19_local_count}\nWorker: ${q19_worker_count}" >> ${Mail_File}
echo -e "WORKER Q19 SYNC DONE\nDone Exiting..."


cat ${Mail_File} | mailx -s "ORS REPLY BACKUP WORKER : ${YYYYMMDD} " -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in 

#Complaince reports

comp_local_dir="/run/media/dvcinfra/DATA/reports/Compliance/NSE"
comp_worker_dir="/NAS4/data/reports/Compliance/"

rsync -avz --progress --no-owner --no-group ${comp_local_dir}  ${WORKER_USER}@${Q19_WORKER_SERVER}:${comp_worker_dir}
