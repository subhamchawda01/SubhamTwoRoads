#!/bin/bash

notis_dir="/home/dvctrader/important/NOTIS_FILES/"

YYYYMMDD=`date +"%Y%m%d"`
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4};
time_=`date -d "+ 330 minutes" +'%H:%M'`
user="DELL"

sshpass -p 'tworoads321$' ssh $user@10.23.5.20 "dir C:\\Users\\${user}\\Desktop\\Postman_Position_File\\CM\\${YYYY}\\${MM}\\${DD}" | grep comb.txt | tail -n1 | awk '{print $4}' >/tmp/file_details
echo "sshpass -p 'tworoads321$' ssh $user@10.23.5.20 \"dir C:\\Users\\${user}\\Desktop\\Postman_Position_File\\CM\\${YYYY}\\${MM}\\${DD}\""
sed -i 's/\r//g' /tmp/file_details

File_name=`cat /tmp/file_details | cut -d' ' -f1`
old_file="Trade_export_CM_${YYYYMMDD}_old_5min.txt"

echo "FILE NOTIS:    $notis_dir$File_name"
echo "OLD FILE NOTIS:    $notis_dir$old_file"

cd $notis_dir
touch $File_name
mv $File_name $old_file

sshpass -p 'tworoads321$' scp $user@10.23.5.20:"C:\\Users\\${user}\\Desktop\\Postman_Position_File\\CM\\${YYYY}\\${MM}\\${DD}\\${File_name}" $notis_dir
echo "DONE 2"
if [[ $? -ne 0 ]]; then 
  echo "Scp Failed To neat server"
  echo "" | mailx -s "Scp Failed To Neat server $File_name $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit -1
fi

if [ ! -s "$File_name" ]; then
  echo "File not Exist"
  echo "" | mailx -s " CM tradeExport File Not Exist $File_name $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit -1
fi
echo "DONE 3"
dif_count=`diff $File_name $old_file |wc -l`

if [[ $dif_count -eq 0 ]]; then
  echo "File Not Updated in the logs"
   HHMM=`date +"%H%M"`
  if [ ${HHMM} -lt 1000 ];
  then
      echo "MAIL...";
      echo "" | mailx -s "CM tradeExport File Not Updated $File_name $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  fi
fi

echo "Success File Updated at $time_ "
echo "DONE 4"
sshpass -p 'tworoads321$' ssh $user@10.23.5.20 "dir C:\\Users\\${user}\\Desktop\\Postman_Position_File\\CM\\${YYYY}\\${MM}\\${DD}" | grep output_file_ | tail -n1 | awk '{print $4}' >/tmp/file_details

sed -i 's/\r//g' /tmp/file_details
out_file=`cat /tmp/file_details | cut -d' ' -f1`

echo "OUTFILE:    $out_file"

sshpass -p 'tworoads321$' scp $user@10.23.5.20:"C:\\Users\\${user}\\Desktop\\Postman_Position_File\\CM\\${YYYY}\\${MM}\\${DD}\\${out_file}" $notis_dir

last_update=`cat $out_file | grep time | grep $YYYY | tail -n1`
echo "NOTIS LAST UPDATE: $last_update"



