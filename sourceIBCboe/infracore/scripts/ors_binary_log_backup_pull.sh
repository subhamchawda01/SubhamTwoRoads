#!/bin/bash
USAGE="$0    EXCHANGE(IND13)    CURRENT_LOCATION(5.67)  YYYYMMDD";
DEST_SERVER="10.23.227.63"
USER="dvcinfra"
Mail_File="/tmp/ors_binary_mail"
Local_Dir="/NAS1/data/ORSData"
Remote_Dir="/spare/local/ORSBCAST"

send_error(){
		echo "" | mailx -s "ORS Binary Backup Error: $DEST_SERVER Unreachable" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" infra_alerts@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
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
echo "Computing for Date: $YYYYMMDD"

#check_dest_alive;
#remote_count=`ssh $DEST_SERVER "ls $Remote_Dir/$EXCHANGE/ | grep $YYYYMMDD | wc -l"`
#echo "$remote_count"
#[[ $remote_count -lt 500  ]] && ssh $USER@$DEST_SERVER 'bash -s' < /home/dvcinfra/important/ors_binary_log_convert.sh $EXCHANGE $YYYYMMDD;
log_convert_running=`ssh $DEST_SERVER "ps aux| grep '/home/pengine/prod/live_scripts/ors_binary_log_convert.sh' | grep -v 'grep'|wc -l"`
while [ $log_convert_running != 0 ]; do
      sleep 10 ;
      echo "Data not converted on IND13"
      echo "Running Convertion script on IND13"
      log_convert_running=`ssh $DEST_SERVER "ps aux| grep '/home/pengine/prod/live_scripts/ors_binary_log_convert.sh' | grep -v 'grep'|wc -l"`
      [[ $? != 0 ]] && sleep 10m;
done

mkdir -p "$Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD"

local_count=`ls "$Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD/" | wc -l`
remote_count=`ssh $DEST_SERVER "ls $Remote_Dir/$EXCHANGE/*$YYYYMMDD* | wc -l"`
echo "Remote_Count: $remote_count"
echo "Local Count: $local_count"
i=0;
while [[ $local_count -ne $remote_count ]] ; do 
        i=$((i+1))
        [[ $i > 4 ]] && { send_error; break; }
        echo "Syncing Files..."
	rsync -ravz "$DEST_SERVER:$Remote_Dir/$EXCHANGE/*$YYYYMMDD*" "$Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD/"
        [[ $? != 0 ]] && sleep 10m;
        local_count=`ls "$Local_Dir/$CURRENT_LOCATION/$YYYY/$MM/$DD/" | wc -l`
done
echo "Done. Exiting..."
