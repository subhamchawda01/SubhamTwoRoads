#YYYYMMDD=`date +"%Y%m%d"`
YYYYMMDD=$1
YY=${YYYYMMDD:2:2}
MM=${YYYYMMDD:4:2}
DD=${YYYYMMDD:6:2}
YYYY=${YYYYMMDD:0:4}
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;
mail_file=/tmp/check_daily_bse_mail_file
rm -rf ${mail_file}

GetPreviousWorkingDay() {
  prev_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_working_day T`
  while [ $is_holiday_ = "1" ];
  do
    prev_working_day=`/home/pengine/prod/live_execs/update_date $prev_working_day P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_working_day T`
  done
}

GetNextWorkingDay() {
  next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N A`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  while [ $is_holiday = "1" ] 
  do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  done
}


GetPreviousWorkingDay;
P_YY=${prev_working_day:2:2}
P_MM=${prev_working_day:4:2}
P_DD=${prev_working_day:6:2}
P_YYYY=${prev_working_day:0:4}
GetNextWorkingDay;

#check price band file
FTP_DIR='/spare/local/files/BSEFTPFiles'
today_file="${FTP_DIR}/$YYYY/$MM/$DD/SCRIP/SCRIP_${DD}${MM}${YY}.TXT"
prev_day_file="${FTP_DIR}/${P_YYYY}/${P_MM}/${P_DD}/SCRIP/SCRIP_${P_DD}${P_MM}${P_YY}.TXT"

diff_count=`diff ${today_file} ${prev_day_file} | wc -l`
([ ! -f ${today_file} ] || [ $diff_count -eq 0 ])&& echo "PRICE BAND FILE NOT UPDATED" > ${mail_file}

#check contract file
today_file="${FTP_DIR}/$YYYY/$MM/$DD/EQD_CO${DD}${MM}${YY}.csv"
prev_day_file="${FTP_DIR}/${P_YYYY}/${P_MM}/${P_DD}/EQD_CO${P_DD}${P_MM}${P_YY}.csv"

diff_count=`diff ${today_file} ${prev_day_file} | wc -l`
([ ! -f ${today_file} ] || [ $diff_count -eq 0 ])&& echo "CONTRACT FILE NOT UPDATED" >> ${mail_file}

Secban_File="/spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_$next_working_day.csv"
[ ! -f $Secban_File ] && echo "Security Under ban file not present" >>${mail_file}

TRADE_INFO_DIR='/spare/local/tradeinfo/BSE_Files'

#check FO bhav copy
bhavcopy_file="${TRADE_INFO_DIR}/BhavCopy/fo/${MM}${YY}/bhavcopy${DD}-${MM}-${YY}.csv";
[ ! -f $bhavcopy_file ] && echo "BHAVCOPY IS NOT PRESENT" >> ${mail_file}

#check CM bhav copy
bhavcopy_file_cm="${TRADE_INFO_DIR}/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV";
[ ! -f $bhavcopy_file_cm ] && echo "BHAVCOPY CM IS NOT PRESENT" >> ${mail_file}

#check margin file
margin_span_file="${TRADE_INFO_DIR}/Margin_Files/Span_Files/BSERISK${YYYYMMDD}-FINAL.spn"
[ ! -f $margin_span_file ] && echo  "MARGIN SPAN FILE NOT PRESENT" >> ${mail_file}


#check security margin file
security_margin_file=${TRADE_INFO_DIR}"/SecuritiesMarginFiles/security_margin_"${next_working_day}".txt"
([ ! -f $security_margin_file ] || [ `cat $security_margin_file | grep FUT | awk '{if(NF!=2){print $0}}' | wc -l` -ne 0  ]) &&  echo "MARGIN FILE PROBLEM" >> ${mail_file}

#check contract file
#contract_file=${TRADE_INFO_DIR}"/ContractFiles/nse_contracts."$next_working_day
#([ ! -f $contract_file ] || [ `cat $contract_file | grep STKFUT |  wc -l` -eq 0  ]) &&  echo "CONTRACT FILE PROBLEM" >> ${mail_file} 
