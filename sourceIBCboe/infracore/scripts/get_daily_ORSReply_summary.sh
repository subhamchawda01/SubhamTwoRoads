#!/bin/bash

if [ "$#" -ne 1 ] ; then
  echo "USAGE: SCRIPT <YYYYMMDD/YESTERDAY>"
  exit
fi

yyyymmdd=$1

if [ $1 == "YESTERDAY" ]; then
  echo "YESTERDAY"
  yyyymmdd=`date -d"1 day ago" +%Y%m%d`
fi

date=$yyyymmdd
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $yyyymmdd T`
if [ $is_holiday = "1" ] ; then
  date=`/home/pengine/prod/live_execs/update_date $yyyymmdd P W`
fi

echo "date:: $date"
YYYY=${date:0:4}
MM=${date:4:2}
DD=${date:6:2}
USER="dvcinfra"
IND13_SERVER="10.23.227.63"
DEST_SERVER="10.23.5.67"
BACKUP_SERVER="10.23.5.42"
ORSREPLY_CONVERTED_DATA_DIR="/spare/local/ORSBCAST/NSE"
ORSREPLY_GENERIC_DIR="/spare/local/MDSlogs/ORS_OLD_$date"
DEST_ORSREPLY_DIR="/NAS1/data/ORSData/NSE/$YYYY/$MM/$DD"

ORSBCAST_MULTISHM_DIR="/spare/local/ORSBCAST_MULTISHM/NSE"
BACKUP_ORSREPLY_DIR="/run/media/dvcinfra/NEWBACKUP/ORSBCAST_MULTISHM_Q19"

MAIL_FILE="/tmp/daily_ORSREPLY_Summary_mail"

>$MAIL_FILE

echo "$YYYY:$MM:$DD"
echo "$ORSREPLY_GENERIC_DIR"
echo "$ORSREPLY_CONVERTED_DATA_DIR"
echo "$DEST_ORSREPLY_DIR"

ORSREPLY_CONVERTED_DATA_COUNT=`ssh $USER@$IND13_SERVER "ls $ORSREPLY_CONVERTED_DATA_DIR | grep $date | wc -l"`
ORSREPLY_GENERIC_COUNT=`ssh $USER@$IND13_SERVER "ls $ORSREPLY_GENERIC_DIR | grep ORS_NSE | wc -l"`
LOCAL_SERVER_COUNT=`ls $DEST_ORSREPLY_DIR | wc -l`

ORSBCAST_MULTISHM_COUNT=`ssh $USER@$IND13_SERVER "ls $ORSBCAST_MULTISHM_DIR | grep $date | wc -l"`
BACKUP_SERVER_COUNT=`ssh $USER@$BACKUP_SERVER "ls $BACKUP_ORSREPLY_DIR | grep $date | wc -l"`


echo "$ORSREPLY_GENERIC_COUNT : $ORSREPLY_CONVERTED_DATA_COUNT : $LOCAL_SERVER_COUNT : $ORSBCAST_MULTISHM_COUNT : $BACKUP_SERVER_COUNT"

#ORSREPLY_CONVERTED_DATA_COUNT=10

if (( $ORSREPLY_CONVERTED_DATA_COUNT == $ORSREPLY_GENERIC_COUNT  && $ORSREPLY_GENERIC_COUNT == $LOCAL_SERVER_COUNT && $ORSREPLY_GENERIC_COUNT != 0)); then
  echo "ORS_REPLY DATA COPY DONE" >>$MAIL_FILE
elif (( $ORSREPLY_CONVERTED_DATA_COUNT < $ORSREPLY_GENERIC_COUNT )); then
  echo "PARTIAL ORS_REPLY conversion happened" >>$MAIL_FILE
elif (( $LOCAL_SERVER_COUNT < $ORSREPLY_GENERIC_COUNT )); then
  echo "NEED TO DO RSYNC for 5.67" >>$MAIL_FILE
fi

if (( $ORSBCAST_MULTISHM_COUNT == $BACKUP_SERVER_COUNT )); then
  echo "ORSBCAST_MULTISHM DATA COPY DONE" >>$MAIL_FILE
elif (( $BACKUP_SERVER_COUNT < $ORSREPLY_GENERIC_COUNT )); then
  echo "NEED TO DO RSYNC for 5.42" >>$MAIL_FILE
fi

echo -e "\nGENERIC COUNT                       : $ORSREPLY_GENERIC_COUNT">>$MAIL_FILE
echo "CONVERTED_DATA_COUNT     : $ORSREPLY_CONVERTED_DATA_COUNT">>$MAIL_FILE
echo "LOCAL_SERVER_COUNT(5.67) : $LOCAL_SERVER_COUNT" >>$MAIL_FILE

echo -e "\nORSBCAST_MULTISHM_COUNT : $ORSBCAST_MULTISHM_COUNT" >>$MAIL_FILE  
echo "BACKUP_SERVER_COUNT(5.42) : $BACKUP_SERVER_COUNT" >>$MAIL_FILE

cat $MAIL_FILE | mailx -s "DAILY ORS_REPLY SUMMARY [$date]" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
