#!/bin/bash
# run it after Fetch nse
print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

get_expiry_date () {
  ONE=01;
  EXPIRY=$YYYYMMDD;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

#Main
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

YYYYMMDD=$1
YYYY=${YYYYMMDD:0:4}
MM=${YYYYMMDD:4:2}
get_expiry_date;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] || [ "$YYYYMMDD" != "$EXPIRY"  ]; then
   echo "NSE Holiday. OR Not Expiry day Exiting...";
   exit;
fi
next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ]
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
#copy security margin file of today to next day
cp /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_$YYYYMMDD.txt /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_$next_working_day.txt

cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day | grep -v $YYYY$MM > /home/dvctrader/trash/nse_contracts.$next_working_day

mv /home/dvctrader/trash/nse_contracts.$next_working_day /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day

#update midterm db, since contract files has changed
cp /spare/local/tradeinfo/NSE_Files/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db.bkp
cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db
/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/Update_MidTermDB_Lotsizes.py  --date $next_working_day --db_path /home/dvctrader/trash/midterm_db
mv /home/dvctrader/trash/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db

chown :infra /spare/local/tradeinfo/NSE_Files/midterm_db
chown :infra /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day

