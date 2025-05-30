#!/bin/bash

if [ $# -ne 1 ] ; then
  echo "Called As : YYYYMMDD" ;
  exit ;
fi


today=$1;
YYYYMMDD=$1
yyyy=${today:0:4}
YYYY=${today:0:4}
mm=${today:4:2}
dd=${today:6:2}
mailfile=/tmp/data_db_report.html_temp_txt;
mail_report=$mailfile
>>$mailfile;
db_date="${yyyy}-${mm}-${dd}"
echo "DB DATE: "$db_date
echo "DB DATE: "$db_date >>$mailfile
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
FILE="/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/fo"$DD"$MSTR"$YYYY"bhav.csv"
echo "chekcing for Fo file $FILE"
while [[ ! -f "$FILE" ]]; do
    echo "$FILE not exists."
    sleep 5m;
done
echo "running"
echo "DB UPDATE COMPLTED"
data_temp=/tmp/temp_Data_logs

mysql -D NSE_MTBT_MIN -u root -h 54.90.155.232 -p'pu2@n6db' -e "select count(*) from MKT_DAILY_MIN_DATA_CASH where day = \"$db_date\"" | tail -1 >$data_temp
echo "MIN CASH COUNT:-     `cat $data_temp`"
echo "MIN CASH COUNT:-     `cat $data_temp`" >> $mailfile
mysql -D NSE_MTBT_MIN -u root -h 54.90.155.232 -p'pu2@n6db' -e "select count(*) from MKT_DAILY_MIN_DATA_FUT where day = \"$db_date\"" | tail -1 >$data_temp
echo "MIN FUT COUNT:-     `cat $data_temp`"
echo "MIN FUT COUNT:-     `cat $data_temp`" >> $mailfile

mysql -D NSE_MTBT -u root -h 54.90.155.232 -p'pu2@n6db' -e "select count(*) from MKT_DAILY_DATA  where day = \"$db_date\"" | tail -1 >$data_temp
echo "DAY CASH AND FUT COUNT:-     `cat $data_temp`"
echo "DAY CASH AND FUT COUNT:-     `cat $data_temp`" >> $mailfile
mysql -D NSE_MTBT -u root -h 54.90.155.232 -p'pu2@n6db' -e "select count(*) from BHAV_COPY_DETAILS_CASH where timestamp = \"$db_date\"" | tail -1 >$data_temp
echo "CM BHAVDAY COUNT:-           `cat $data_temp`"
echo "CM BHAVDAY COUNT:-           `cat $data_temp`" >> $mailfile
mysql -D NSE_MTBT -u root -h 54.90.155.232 -p'pu2@n6db' -e "select count(*) from BHAV_COPY_DETAILS_FO where timestamp = \"$db_date\"" | tail -1 >$data_temp
echo "FO BHAVCOPY COUNT:-          `cat $data_temp`"
echo "FO BHAVCOPY COUNT:-          `cat $data_temp`" >> $mailfile
mysql -D NSE_MTBT -u root -h 54.90.155.232 -p'pu2@n6db' -e "select count(*) from INDICATOR_DAILY_DATA where timestamp = \"$db_date\"" | tail -1 >$data_temp
echo "TECHINCAL INDICATOR COUNT:-   `cat $data_temp`"
echo "TECHINCAL INDICATOR COUNT:-   `cat $data_temp`" >> $mailfile

echo 
#cat $mailfile | /bin/mail -s "DB WORKER JOBS FOR ${today} COMPLETED" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in
