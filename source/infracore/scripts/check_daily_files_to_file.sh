#YYYYMMDD=`date +"%Y%m%d"`
YYYYMMDD=$1
YY=${YYYYMMDD:2:2}
MM=${YYYYMMDD:4:2}
DD=${YYYYMMDD:6:2}
YYYY=${YYYYMMDD:0:4}
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;
mail_file=/tmp/check_daily_mail_file
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
GetNextWorkingDay;

#check price band file
FTP_DIR='/spare/local/files/NSEFTPFiles'
today_file=${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}"/security"
prev_day_file=${FTP_DIR}"/"${prev_working_day:0:4}"/"${prev_working_day:4:2}"/"${prev_working_day:6:2}"/security"

diff_count=`diff ${today_file} ${prev_day_file} | wc -l`
([ ! -f ${today_file} ] || [ $diff_count -eq 0 ])&& echo "PRICE BAND FILE NOT UPDATED" > ${mail_file}

#secband extra check
cat $today_file | grep EQ | cut -d'|' -f2,7 | sort >/tmp/fsec1
cat $prev_day_file | grep EQ | cut -d'|' -f2,7 | sort >/tmp/fsec2
diff_count=`diff /tmp/fsec1 /tmp/fsec2 | wc -l`
echo $diff_count
([ ! -f ${today_file} ] || [ $diff_count -lt 3500 ]) && echo "PRICE BAND FILE NOT UPDATEDFAILED 2nd check" >> ${mail_file}

#check contract file
FTP_DIR='/spare/local/files/NSEFTPFiles'
today_file=${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}"/contract"
prev_day_file=${FTP_DIR}"/"${prev_working_day:0:4}"/"${prev_working_day:4:2}"/"${prev_working_day:6:2}"/contract"

diff_count=`diff ${today_file} ${prev_day_file} | wc -l`
([ ! -f ${today_file} ] || [ $diff_count -eq 0 ])&& echo "CONTRACT FILE NOT UPDATED" >> ${mail_file}

Secban_File="/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_$next_working_day.csv"
[ ! -f $Secban_File ] && echo "Security Under ban file not present" >>${mail_file}

TRADE_INFO_DIR='/spare/local/tradeinfo/NSE_Files'

#check FO bhav copy
bhavcopy_file=${TRADE_INFO_DIR}"/BhavCopy/fo/"${MM}${YY}"/fo"${DD}"${MSTR}"${YYYY}"bhav.csv";

[ ! -f $bhavcopy_file ] && echo "BHAVCOPY IS NOT PRESENT" >> ${mail_file}

#check CM bhav copy
bhavcopy_file_cm=${TRADE_INFO_DIR}"/Margin_Files/Exposure_Files/cm"${DD}"${MSTR}"${YYYY}"bhav.csv";

[ ! -f $bhavcopy_file_cm ] && echo "BHAVCOPY CM IS NOT PRESENT" >> ${mail_file}

#check margin file
margin_span_file=${TRADE_INFO_DIR}"/Margin_Files/Span_Files/nsccl."${YYYYMMDD}".s_1.spn"

exposure_margin_rate_file=${TRADE_INFO_DIR}"/Margin_Files/Exposure_Files/exposure_margin_rates."${YYYYMMDD}

[ ! -f $margin_span_file ] && echo  "MARGIN SPAN FILE NOT PRESENT" >> ${mail_file}

[ ! -f $exposure_margin_rate_file ] && echo  "EXPOSURE MARGIN RATES FILE NOT PRESENT" >> ${mail_file}

#check db updated
TEMP_DB='/tmp/check_db_updated'
python /home/dvctrader/important/onexpiry/checkdb_update.py $next_working_day /spare/local/tradeinfo/NSE_Files/midterm_db > $TEMP_DB
[ -s $TEMP_DB ] && echo "MIDTERM DB PROBLEM" >> ${mail_file}

#check security margin file
security_margin_file=${TRADE_INFO_DIR}"/SecuritiesMarginFiles/security_margin_"${next_working_day}".txt"

([ ! -f $security_margin_file ] || [ `cat $security_margin_file | grep FUT | awk '{if(NF!=2){print $0}}' | wc -l` -ne 0  ]) &&  echo "MARGIN FILE PROBLEM" >> ${mail_file}

#check contract file
contract_file=${TRADE_INFO_DIR}"/ContractFiles/nse_contracts."$next_working_day
([ ! -f $contract_file ] || [ `cat $contract_file | grep STKFUT |  wc -l` -eq 0  ]) &&  echo "CONTRACT FILE PROBLEM" >> ${mail_file} 
