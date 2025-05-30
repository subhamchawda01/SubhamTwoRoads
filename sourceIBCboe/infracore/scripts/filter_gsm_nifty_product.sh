#!/bin/bash

if [ "$#" -ne 1 ] ; then
  echo "USAGE: SCRIPT <YYYYMMDD>"
  exit
fi

mail_file=/tmp/mail_gsm_nifty
>$mail_file

dt=$1
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $dt T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

dd=${dt:6:2}
yyyy=${dt:0:4}
mstr=`date -d $dt +%b`
#dt="20201112"
gsm_file="gsm-latest-$dd-$mstr-$yyyy.csv"
nifty_50_file="MW-NIFTY-50-$dd-$mstr-$yyyy.csv"
nifty_next_50_file="MW-NIFTY-NEXT-50-$dd-$mstr-$yyyy.csv"
gsm_nifty_file_path="/home/dvcinfra/important/GsmNiftyFiles/"

if [ ! -f  "${gsm_nifty_file_path}${gsm_file}" ] || [ ! -f  "${gsm_nifty_file_path}${nifty_50_file}" ] || [ ! -f  "${gsm_nifty_file_path}${nifty_next_50_file}" ]; then
  echo "${gsm_nifty_file_path}${gsm_file} -> https://www.nseindia.com/api/reportGSM?csv=true " >> $mail_file 
  echo "${gsm_nifty_file_path}${nifty_50_file} -> https://www.nseindia.com/api/equity-stockIndices?csv=true&index=NIFTY%2050 " >> $mail_file 
  echo "${gsm_nifty_file_path}${nifty_next_50_file} -> https://www.nseindia.com/api/equity-stockIndices?csv=true&index=NIFTY%20NEXT%2050 " >> $mail_file
  cat $mail_file | mailx -s "************ GSM NIFTY FILE MISSING ALERT ***********" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit;
fi

echo "${gsm_nifty_file_path}${gsm_file}"
#awk 'NR>1' "${gsm_nifty_file_path}${gsm_file}" | awk -F "|" '{print $1}' > /tmp/productlist${dt}.txt
awk 'NR>6' "${gsm_nifty_file_path}${gsm_file}" | awk -F "\"*,\"*" '{print $2}' > /tmp/productlist${dt}.txt

echo "${gsm_nifty_file_path}${nifty_50_file}"
#awk 'NR>1' "${gsm_nifty_file_path}${nifty_50_file}" | awk -F "\"*,\"*" '{ if(substr($1,2) != "NIFTY NEXT 50" && substr($1,2) != "NIFTY 50") print substr($1,2)}'  >> /tmp/productlist${dt}.txt
awk 'NR>14' "${gsm_nifty_file_path}${nifty_50_file}" | awk -F "\"*,\"*" '{ if(substr($1,2) != "NIFTY NEXT 50" && substr($1,2) != "NIFTY 50") print substr($1,2)}'  >> /tmp/productlist${dt}.txt

echo "${gsm_nifty_file_path}${nifty_next_50_file}"
awk 'NR>14' "${gsm_nifty_file_path}${nifty_next_50_file}" | awk -F "\"*,\"*" '{ if(substr($1,2) != "NIFTY NEXT 50" && substr($1,2) != "NIFTY 50") print substr($1,2)}' >> /tmp/productlist${dt}.txt

cat /tmp/productlist${dt}.txt | sort | uniq > /NAS1/data/MFGlobalTrades/ind_pnls/CM/trans_add_products/data_nifty_next.${dt}

rm -f /tmp/productlist${dt}.txt
