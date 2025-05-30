#!/bin/bash

if [ "$#" -ne 1 ] ; then
  echo "USAGE: SCRIPT <date>"
  exit
fi

#DATE=`date +"%Y%m%d"`
DATE=$1

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $DATE T`
if [ $is_holiday = "1" ] ; then
    exit;
fi

MAIL_FILE=/tmp/mail_ind13_drops
TEMP_DROP_TIME_FILE=/tmp/tmp_drop_time
TEMP_SEC_DROP_COUNT_FILE=/tmp/tmp_time_drop_count
SHM_WRITER_LOG_DBG="/spare/local/MDSlogs/nseshm_writer_dbg_${DATE}.log"
SHM_WRITER_LOG="/spare/local/MDSlogs/nseshm_writer_dbg_${DATE}.log_read"
OUTPUT_FILE="/home/dvcinfra/important/IND13_DROPS/ind13_drop_summary_${DATE}"
CONVERT_EXEC="/home/dvcinfra/important/IND13_DROPS/convert_sec_date"
declare -a arr_fd
declare -A socket_fd_to_ip_port
declare -A socket_fd_to_type
>$MAIL_FILE
>$OUTPUT_FILE
>$TEMP_DROP_TIME_FILE
>$TEMP_SEC_DROP_COUNT_FILE
cp $SHM_WRITER_LOG_DBG $SHM_WRITER_LOG 
sed -i 's/FD :/FD :IND13-/g' $SHM_WRITER_LOG
sed -i 's/FROM SOCKET : /FROM SOCKET : IND13-/g' $SHM_WRITER_LOG
ssh 10.23.227.66 "cat $SHM_WRITER_LOG_DBG" >>$SHM_WRITER_LOG
echo -e "DATE : $DATE\n" | tee -a $OUTPUT_FILE
count=0
for FD in `grep -w "SOCKET FD" $SHM_WRITER_LOG | awk '{print substr($7,2)}'`;
do
  arr_fd[$count]=$FD;
  #echo "FD : ${arr_fd[$count]}"
  ((++count))
done

grep "DROPPED" $SHM_WRITER_LOG | awk '{ msg_drop+=$25; drop_occ+=1;} END {printf "TOTAL DROPS OCCURRENCES: %d\nTOTAL PKT DROPPED: %d\n",drop_occ,msg_drop}' | tee -a $OUTPUT_FILE $MAIL_FILE


echo -e "\n--------------------------CHANNEL DETAILS--------------------------\n" | tee -a $MAIL_FILE
count=0
for IP_PORT_TYPE in `grep -w "CHANNEL" $SHM_WRITER_LOG | awk '{print $7":"$9","$12}'`;
do
  #echo "$IP_PORT_TYPE"
  IP_PORT=`echo $IP_PORT_TYPE | cut -d',' -f1`;
  TYPE=`echo $IP_PORT_TYPE | cut -d',' -f2`;
  FD=${arr_fd[$count]}
  #echo "IP_PORT : $IP_PORT TYPE : $TYPE FD : $FD"
  socket_fd_to_ip_port[$FD]=$IP_PORT
  socket_fd_to_type[$FD]=$TYPE
  echo "FD : ${arr_fd[$count]} IP_PORT : ${socket_fd_to_ip_port[$FD]} TYPE : ${socket_fd_to_type[$FD]}" | tee -a $OUTPUT_FILE $MAIL_FILE
  ((++count))
done

echo | tee -a $MAIL_FILE
grep -w "DROPPED" $SHM_WRITER_LOG | awk '{ soc[$10]+=1; msg_d[$10]+=($25)} END {for (i in soc){print "SOCKET_FD["i"]"," OCCURRENCE: "soc[i],"\tPKT_DROPPED: "msg_d[i]}}' | sort -k5 -n | tee -a $OUTPUT_FILE $MAIL_FILE

echo -e "\n-----------------------DROP OCCURRENCES --------------------------\n" | tee -a $OUTPUT_FILE $MAIL_FILE

grep -w "DROPPED" $SHM_WRITER_LOG | awk '{print $NF}' >> $OUTPUT_FILE

grep -w "DROPPED" $SHM_WRITER_LOG | awk '{print $NF,$25}' > $TEMP_SEC_DROP_COUNT_FILE

$CONVERT_EXEC $TEMP_SEC_DROP_COUNT_FILE $TEMP_DROP_TIME_FILE;
 

echo "TIME(GMT)  DROP_OCCURANCES  PKT_DROPPED" | tee -a $MAIL_FILE
awk '{occ_[$1]+=1; msg_[$1]+=$2;} END {for (i in occ_) {printf"%s\t\t\t\t%d\t\t\t\t%d\n",i,occ_[i],msg_[i]}}' $TEMP_DROP_TIME_FILE | sort -k1 | tee -a $MAIL_FILE

echo -e "\nPATH: /home/dvcinfra/important/IND13_DROPS" | tee -a $MAIL_FILE

echo ""| mailx -s "IND13 & IND22 DROPS $DATE" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nseall@tworoads.co.in  < $MAIL_FILE

#generate HTML PAGE http://10.23.227.63/nseshm_drops_report/index.html
/home/pengine/prod/live_scripts/drops_summary_web_page.sh

