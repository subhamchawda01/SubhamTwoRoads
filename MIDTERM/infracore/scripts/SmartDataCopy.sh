#!/bin/bash
#Updated central datacopy script. Uses a config file to decide which exchange's data from which location goes to which folder on NAS
USAGE1="$0 EXCHSOURCE DATE"
EXAMP1="$0 EUREX 20140130"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

#Location (from which we want to copy data)
SOURCE=$1 ; shift ;
#Date for which data is to be copied
YYYYMMDD=$1 ; shift ;
#Config file of format "LOCATION EXCHANGE NAS_FOLDER(without_LoggedData_Suffix) IP
CFG_FILE=/home/pengine/prod/live_configs/data_copy.cfg

#~/LiveExec/scripts/logdate.sh $SOURCE

LOGDATE=`echo $YYYYMMDD` ;

if [ "$LOGDATE" == "TODAY" ] ; then 
  LOGDATE=`date +"%Y%m%d"` ;
fi 

#Copy ORS binary and MDS logs
while read line;
do
  EXCHANGE=`echo $line | awk '{print $2}'`
  NAS_FOLDER=`echo $line | awk '{print $3}'`
  IP=`echo $line | awk '{print $4}'`
  if [ "$NAS_FOLDER" == "ORS" ]; then
    >/tmp/ors_data_copy_$IP"_"$EXCHANGE"_"$SOURCE"_"$LOGDATE".dat" ;
    ssh -n -f $IP "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh $EXCHANGE $SOURCE '$YYYYMMDD' >/tmp/ors_data_copy_$EXCHANGE'_'$SOURCE'_'$YYYYMMDD'.cout' 2>/tmp/ors_data_copy_$EXCHANGE'_'$SOURCE'_'$YYYYMMDD'.cerr'" &
  else
    >/tmp/mds_data_copy_$IP"_"$EXCHANGE"_"$SOURCE"_"$LOGDATE".dat" ;
    ssh -n -f $IP "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh $EXCHANGE $SOURCE '$YYYYMMDD' >/tmp/mds_data_copy_$EXCHANGE'_'$SOURCE'_'$YYYYMMDD'.cout' 2>/tmp/mds_data_copy_$EXCHANGE'_'$SOURCE'_'$YYYYMMDD'.dat'" &
  fi
  sleep 1
done < <(grep "^$SOURCE" $CFG_FILE | grep -v "^#")

#start for each location
/home/dvcinfra/LiveExec/scripts/trigger_sampledata_computation.sh  $SOURCE >> /tmp/datacopy_sampledata_log_$SOURCE.$LOGDATE &
/home/dvcinfra/LiveExec/scripts/trigger_results_computation.sh $SOURCE $YYYYMMDD >> /tmp/datacopy_log_$SOURCE.$LOGDATE
