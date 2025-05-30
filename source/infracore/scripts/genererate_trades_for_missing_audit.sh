#!/bin/bash
echo "GENERATE AUDIT FOR MISSING entries "

if [ "$#" -lt 3 ] ; then
  echo "USAGE: SCRIPT <audit_exchange_file 22112021_90044.txt> <audit_ind final_audit_fo_20211122.csv> <date>"
  exit
fi


audit_exchange=$1
audit_ind=$2
YYYYMMDD=$3
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4};
contract_file="/spare/local/files/NSEFTPFiles/$YYYY/$MM/$DD/contract"

>/tmp/missing_trades.txt
while read line; do 
	fill_=`echo $line| awk -F',' '{print $1}'` 
	count_=`grep $fill_ $audit_ind |wc -l`
        if [[ $count_ -eq 0 ]]; then 
	       echo $line >>/tmp/missing_trades.txt
        fi
done < $audit_exchange

>/tmp/missing_trades_to_format.txt
while read line; do
	buy=`echo $line| awk -F',' '{print $9}'`
	p="S"
	if [[ $buy -eq 1 ]]; then 
		p="B";
	fi
	short_=`echo $line | awk -F',' '{print $8}'`
	type_="C"
	if [[ $short_ =~ .*"PE" ]]; then
		type_="P"
	fi
#	date_=`echo $line | awk -F',' '{print $24}'`
#	time_=`date -d"$date_" +"%Y%m%d %H:%M"`
#	echo $time_
	token_=`grep $short_ $contract_file | cut -d'|' -f1`
	strike_=`echo $line | awk -F',' '{print $6}' | sed 's/\.//g' | awk '{print $1}'`
	
#	echo "TOKEN: $token_ BUY: $p Strike: $strike_ Date: $date_"
 	echo $line | awk -F',' -v ty=$type_ -v stl=$strike_ -v bu=$p -v token=$token_ '{print "20222,"$24","$11",6412350917-16:05:42,"$20",4,4.83266E+18,90044,90044,"$13","$14",0,0,0,"$15",0,"$1","$14","$15","$14","bu","$24","token","$4","$3",1322317800,"stl","ty",O,1,90044,"$14*$15}'
done < /tmp/missing_trades.txt


#RUN ./genererate_trades_for_missing_audit.sh | sed "s/22-Nov-2021 /20211122-/g" | sed "s/22 Nov 2021 /20211122-/g" >missing_trades_for_20211121.csv
#happy

''' exchange
Trade ID
Activity type
Symbol
Instrument Type
Expiry Date
Strike Price
Option Type
Contract Name
Book Type
Market Type
User Id
Branch Id
Buy/Sell
Quantity
Price
Pro/Cli
Account number
Participant
Settlement
Trade Date/Time
Modified Date/Time
Order Number
CP ID
Entry Date/Time
CTCL ID'''


''' IND
Transaction Code 20222
Log Time 20211122-12:34:08
Trader ID 36474
Time Stamp 6412331224-16:22
Time stamp1 20211122-12:34
Time stamp2 4
Response order no 4.83266E+18
Broker ID 90044
Account No 90044
Buy/Sell 1
Original_Vol 1100
Disclosed_Val 0
Remain_Vol 0
Disclosed_Val_ Remain 0
Price 1.65
Good_Till_Date 0
Fill No 200844733
Fill Qty 1100
Fill Price 1.65
Vol_Filled _price 1100
Activity Type B
Activity Time 20211122-12:34:08
Token 60411
Instrument OPTSTK
Symbol CADILAHC
Expiry 1322317800
Strike Price 48000
Option Type C
Open_Close O
Book type 1
Participant 90044
Trade_Val 1815
'''
