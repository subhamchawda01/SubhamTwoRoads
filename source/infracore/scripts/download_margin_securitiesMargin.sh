#!/bin/bash

margin_exec_="/home/pengine/prod/live_execs/calc_nse_span"
exch_sym_exec_="/home/pengine/prod/live_execs/get_exchange_symbol"
symbols="/home/pengine/prod/live_configs/nse_midterm_shortcodes"
ds_sym_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
temp_shc_file_="/home/dvctrader/trash/temp_shcs"

FTP_HOST='ftp.connect2nse.com'

download_margin() {
 wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/archives/nsccl/span/nsccl.$YYYY$MM$DD.s.zip
 unzip -o nsccl.$YYYY$MM$DD.s.zip
 rm nsccl.$YYYY$MM$DD.s.zip
 chmod 666 nsccl.$YYYY$MM$DD.s.spn
 mv nsccl.$YYYY$MM$DD.s.spn nsccl.$YYYY$MM$DD.s_1.spn
}

download_exposure(){
 cd /spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files
 wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/archives/exp_lim/ael_$DD$MM$YYYY.csv
 cat ael_$DD$MM$YYYY.csv | awk -F "," '{print $1,$2,$5}' > exposure_margin_rates.$YYYY$MM$DD
 rm ael_$DD$MM$YYYY.csv
}

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

cash_margin(){
 month_name=`date -d $YYYY$MM$DD '+%b' | awk '{print toupper($0)}'`
 wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/archives/nsccl/var/C_VAR1_"$DD$MM$YYYY"_6.DAT
 wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/content/historical/EQUITIES/"$YYYY"/"$month_name"/cm"$DD""$month_name""$YYYY"bhav.csv.zip

 if [ ! -f cm"$DD""$month_name""$YYYY"bhav.csv.zip ] || [ ! -f C_VAR1_"$DD$MM$YYYY"_6.DAT ]; then
   error_msg="Unable to download Bhavcopy file for $YYYYMMDD. Please download the file manually"
   echo "" | mailx -s "CVAR1 and Bhavcopy for Unable to download" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in
   exit
 fi

 unzip cm"$DD""$month_name""$YYYY"bhav.csv.zip
 rm cm"$DD""$month_name""$YYYY"bhav.csv.zip

 awk -F"," '$3 == "EQ" {print $2","$10}'  C_VAR1_"$DD$MM$YYYY"_6.DAT > cm_loss_rate_"$DD$MM$YYYY"
 awk -F"," '$2 == "EQ" {print $1","$6}'  cm"$DD""$month_name""$YYYY"bhav.csv > cm_closing_price_"$DD$MM$YYYY"

 awk -F "," 'FNR==NR{a[$1]=$2;next}($1 in a){print $1,a[$1],$2}' OFS="," cm_loss_rate_"$DD$MM$YYYY" cm_closing_price_"$DD$MM$YYYY" | awk -F "," '{print $1" "($2*$3/100)""}' > cm_var_"$DD$MM$YYYY"
 cp cm_var_"$DD$MM$YYYY" cm_var_"$DD$MM$YYYY"_copy
 awk 'BEGIN{OFS=" "}$1="NSE_"$1' cm_var_"$DD$MM$YYYY"_copy > cm_var_"$DD$MM$YYYY"
 cat cm_var_"$DD$MM$YYYY" >> /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt
 cp cm"$DD""$month_name""$YYYY"bhav.csv "/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/cm${DD}${month_name}${YYYY}bhav.csv"
 
 rm cm_var_"$DD$MM$YYYY" cm_var_"$DD$MM$YYYY"_copy cm_loss_rate_"$DD$MM$YYYY" cm_closing_price_"$DD$MM$YYYY" C_VAR1_"$DD$MM$YYYY"_6.DAT
}

options_margin(){
 declare -A hashmap
 for i in `cat $symbols`; do
   if [[ "`echo $i | cut -d'_' -f3 | grep FUT`" == "" ]]; then
    stock_=`echo $i | cut -d'_' -f2`
    if [[ "`echo ${hashmap[$stock_]}`" == "" ]]; then
       ex_sym=`$exch_sym_exec_ $i $next_working_day`
       [ -z $ex_sym ]  && continue;
       ds_sym=`grep $ex_sym $ds_sym_file | awk 'BEGIN{FS=" "}{print $2}'`
       echo -e "$ds_sym\t-1" >$temp_shc_file_
       hashmap[$stock_]=`$margin_exec_ $YYYY$MM$DD $temp_shc_file_ | awk 'BEGIN{FS="_"}{print $1+$2}'`
    fi
    margin=`echo "${hashmap[$stock_]}"`
    echo $i" "$margin >> "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt"
   fi
 done
}

fut_margin(){
 declare -A expmap
 expiries=(`cat $contract_file_ | grep "IDXFUT" | awk 'BEGIN{FS="\t"}{print $6}' | sort -u`)
 for i in `seq 0 $((${#expiries[@]}-1))`; do  expmap[${expiries[i]}]=$i ; done
 for i in `cat $contract_file_ | egrep "IDXFUT|STKFUT" | awk 'BEGIN{FS="\t"; OFS=","}{print $2,$6}'`; do
    stk=`echo $i | cut -d',' -f1`
    exp=`echo $i | cut -d',' -f2`
    ds_sym="NSE_"$stk"_FUT_"$exp
    shc="NSE_"$stk"_FUT"${expmap[$exp]}
    echo -e "$ds_sym\t-1" > $temp_shc_file_
    margin=`$margin_exec_ $YYYY$MM$DD $temp_shc_file_ | awk 'BEGIN{FS="_"}{print $1+$2}'`
    echo $shc" "$margin >> "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt"
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
 echo "NSE Holiday. Exiting...";
   exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4};
next_working_day=`/home/pengine/prod/live_execs/update_date $YYYY$MM$DD N W`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo  "NextDay: $next_working_day";
contract_file_="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day
[[ -f $contract_file ]] && { echo "Contract file not present"; exit; }
securities_dir="/spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/"
cd $securities_dir
[[ $1 == "FORCE" ]] && `rm -rf "fo"${DD}${MSTR}${YYYY}"bhav.csv"`
[[ $1 == "FORCE" ]] && `rm -rf "nsccl.${YYYY}${MM}${DD}.s_1.spn" "security_margin_"$next_working_day".txt"`
[[ $1 == "FORCE" ]] && `rm -rf "/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/exposure_margin_rates.$YYYY$MM$DD"`

[[  -f "nsccl.${YYYY}${MM}${DD}.s_1.spn" ]] && [[ -f "/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/exposure_margin_rates.$YYYY$MM$DD" ]] && [[ -f "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt" ]] && { echo "File exist"; exit;}
[[  -f "nsccl.${YYYY}${MM}${DD}.s_1.spn" ]] || download_margin
[[  -f "/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/exposure_margin_rates.$YYYY$MM$DD" ]] || download_exposure
[[  -f "nsccl.${YYYY}${MM}${DD}.s_1.spn" ]] || { echo "Unable to download Span file exiting..."; exit;}


>"/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt"
#For cash
cash_margin;
cd /spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/ ;
#For options
options_margin;
#For all futures from Contract File
fut_margin;

echo "" | mailx -s "MARGIN & SECURITY FILE UPDATED" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in
find /spare/local/tradeinfo/NSE_Files/DataExchFile -type f -mtime +7 -exec rm -f {} \;
