#!/bin/bash
FTP_DIR="/spare/local/files/NSEFTPFiles";

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

GetNextWorkingDay(){
  next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`
  isholiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  while [[ $is_holiday = "1" ]]
  do
      next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
      is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  done
  echo $next_working_day
}

#Main
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

YYYYMMDD=$1
GetNextWorkingDay;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4};
N_DD=${next_working_day:6:2}
N_MM=${next_working_day:4:2}
N_YY=${next_working_day:2:2}
N_YYYY=${next_working_day:0:4}


cp /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${N_MM}${N_YY}.csv /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${YYYYMMDD}.csv
cp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_${next_working_day}_contracts.txt /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_${YYYYMMDD}_contracts.txt
cp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_${next_working_day}_contracts.txt /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_${YYYYMMDD}_contracts.txt
cp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_${next_working_day}_contracts.txt /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_${YYYYMMDD}_contracts.txt
cp /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${next_working_day}.txt /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${YYYYMMDD}.txt
cp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$YYYYMMDD

/home/dvctrader/important/onexpiry/sync_trade_info.sh
