#!/bin/bash

DDMMYY=$1

YYYYMMDD="20${DDMMYY:4:2}${DDMMYY:2:2}${DDMMYY:0:2}"
mmm=`date -d $YYYYMMDD +%b | tr '[:lower:]' '[:upper:]'`
ddmmmyyyy="${YYYYMMDD:6:2}$mmm${YYYYMMDD:0:4}"

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
    echo "NSE Holiday. Exiting...";
    exit;
fi


nseBhavcopyPath="/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/cm"$ddmmmyyyy"bhav.csv"
bseBhavcopyPath="/spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ"$DDMMYY".CSV"
bseSecurityInfo="/spare/local/tradeinfo/BSE_Files/RefData/bse_eq_"$YYYYMMDD"_contracts.txt"

tmp="~/trash/nseInBseBhav.csv"
destpath="/spare/local/tradeinfo/BSE_Files/BhavCopy/onlyBseEQ/onlyBseBhavcopy$YYYYMMDD.csv"

>$destpath
>tmp

awk 'BEGIN {FS=OFS=","}{gsub(/LTD$/,"",$1)}{gsub(/LIMITED$/,"",$1)}{print $1"^"}' $nseBhavcopyPath | while read line;
do
	awk 'BEGIN{gsub(/LTD$/,"",$4)}{gsub(/LIMITED$/,"",$4)}{print $1" "$4"^"}' $bseSecurityInfo | grep -w "$line" | awk '{print $1}' | while read securitycode;
	do
		grep -w "$securitycode" $bseBhavcopyPath >>tmp
	done
done

grep -v -f tmp $bseBhavcopyPath | sort -t"," -b -n -k13 >$destpath
