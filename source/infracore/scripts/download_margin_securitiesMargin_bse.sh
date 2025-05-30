#!/bin/bash

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

HHMM=`date +"%H%M"`
YYYYMMDD=`date +"%Y%m%d"`
if [ ${HHMM} -lt 1000 ];
then
  GetPreviousWorkingDay;
fi
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "BSE Holiday. Exiting...";
   exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4};
next_working_day=`/home/pengine/prod/live_execs/update_date $YYYY$MM$DD N W`
NXT_DAY_YYYY=${next_working_day:0:4}
NXT_DAY_MM=${next_working_day:4:2}
NXT_DAY_DD=${next_working_day:6:2}
nxt_wor="${NXT_DAY_DD}${NXT_DAY_MM}${NXT_DAY_YYYY}"

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo  "NextDay: $next_working_day";
cd /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles
#no Run file already downloaded
[[ $1 == "FORCE" ]] && `rm -rf "security_margin_"$next_working_day".txt"`
[[ -f "security_margin_"$next_working_day".txt" ]]  && exit;
[[ ! -f "/spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${YYYYMMDD}.csv" ]] && echo "FILE NOT PRESENT /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${YYYYMMDD}.csv" && exit;
[[ ! -f "/spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV" ]] && echo "FILE NOT PRESENT /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV" && exit;

awk -F"," 'NR==FNR{a[$1]=$10; next} ($1 in a){print $1"|"(a[$1]*$8)/100}' /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${YYYYMMDD}.csv /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_FilesEQ${DD}${MM}${YY}.CSV  > /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp

awk -F"|" 'NR==FNR{a[$1]=$3; next} ($1 in a){print "BSE_"a[$1]" "$2}'  $FTP_DIR/SCRIP/SCRIP_${DD}${MM}${YY}.TXT /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp > /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt

rm /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp
verify_given_files /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt
echo "SM File generated"
chgrp infra /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt

year=${next_working_day:0:4}
month=$(date -d $next_working_day +"%b")
next_day=${next_working_day:6:2}

[[ -f "security_margin_"$next_working_day".txt" ]] && echo "" | mailx -s "BSE SM FILE GENERATED" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in

