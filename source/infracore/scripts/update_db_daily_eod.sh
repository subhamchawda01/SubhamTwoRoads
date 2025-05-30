#!/bin/bash
send_slack_exec="/home/pengine/prod/live_execs/send_slack_notification"
#db_generate_exec=/home/dvcinfra/raghu/DB_UPDATE/cvquant_install/baseinfra/bin/db_nse_update_for_day
db_generate_exec="/home/dvctrader/stable_exec/db_nse_update_for_day_Whole_Book_faster"
db_bhav_exec="//home/dvctrader/stable_exec/db_bhavcopy_techincalIndicator_update_for_day"
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'

GetNearestExpiry() {
  date_=$YYYYMMDD
  date_=`/home/pengine/prod/live_execs/update_date $date_ P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
  while [ $is_holiday_ = "1" ];
  do
    date_=`/home/pengine/prod/live_execs/update_date $date_ P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
  done

  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${date_}
  expiry=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${date_} '{if($NF>=date)print $NF'} | sort | uniq | head -n1`
  echo "Expiry $expiry"
  next_working_day=`/home/pengine/prod/live_execs/update_date $expiry N A`

  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  while [[ $is_holiday = "1" ]] 
  do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  done
  echo "Nxt day of Expiry $next_working_day"
}


if [ $# -ne 1 ] ; then
  echo "Called As : " $* "YYYYMMDD" ;
  exit;
fi

echo "---------------------------------------------Starting DB DUMP NSE-------------------------------------"
echo 
YYYYMMDD=$1 ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

MKT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/$YYYY/$MM/$DD/"
PRODUCT_FILE_CASH="/tmp/PRODUCT_FILE_DB_CASH_$YYYYMMDD"
PRODUCT_FILE_FUT="/tmp/PRODUCT_FILE_DB_FUT_$YYYYMMDD"
>$PRODUCT_FILE_CASH
>$PRODUCT_FILE_FUT
i=0;
GetNearestExpiry

echo "RUNNING FOR CASH"
for file in `ls $MKT_DATA_DIR | grep -v _FUT_  | grep -v "_CE_" | grep -v "_PE_" | egrep -iv "HANGSENGBEES|INDIAVIX|NSE_NIFTY"`; do
    product=`echo $file | cut -d'_' -f2` 
    echo "NSE_$product" >>$PRODUCT_FILE_CASH
done
echo "TOTAL PRODUCTS `wc -l $PRODUCT_FILE_CASH`"

head -n1000 $PRODUCT_FILE_CASH  >${PRODUCT_FILE_CASH}_1
sed -i -e '1,1000d' $PRODUCT_FILE_CASH
 
echo "$db_generate_exec $PRODUCT_FILE_CASH $YYYYMMDD"
# clear DB ALSO 
taskset -c 2,3 $db_generate_exec $PRODUCT_FILE_CASH $YYYYMMDD FORCE
pids[${i}]=$!;
i=${i+1};
# sleep so the force doesn't delete new entries of others
#sleep 20

echo "RUNNING FOR CASH2.. "
echo "$db_generate_exec ${PRODUCT_FILE_CASH}_1 $YYYYMMDD"
taskset -c 4,5 $db_generate_exec ${PRODUCT_FILE_CASH}_1 $YYYYMMDD
pids[${i}]=$!;
i=${i+1};

echo "RUNNING FOR FUTURE.. "
# better to check LOTSIZE FILE
for product in `ls $MKT_DATA_DIR | grep _FUT_  | grep -v FINNIFTY | grep -v MIDCPNIFTY | cut -d'_' -f2 | sort | uniq`; do
    product_in_lot=`grep $product /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${YYYYMMDD}.csv | wc -l`
    if [[ $product_in_lot -eq 0 ]]; then
        echo "Product PRODUCT: $product Count: $product_in_lot not in LOTFILE-> /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${YYYYMMDD}.csv"
        continue;
    fi
    echo "NSE_"$product"_FUT0" >>$PRODUCT_FILE_FUT
    echo "NSE_"$product"_FUT1" >>$PRODUCT_FILE_FUT
    if [[ $next_working_day -ne $YYYYMMDD ]]; then 
#    	echo "Including FUT2"
    	echo "NSE_"$product"_FUT2" >>$PRODUCT_FILE_FUT
    fi
done

head -n300 $PRODUCT_FILE_FUT  >${PRODUCT_FILE_FUT}_1
sed -i -e '1,300d' $PRODUCT_FILE_FUT

echo "$db_generate_exec $PRODUCT_FILE_FUT $YYYYMMDD"
taskset -c 6,7 $db_generate_exec $PRODUCT_FILE_FUT $YYYYMMDD
pids[${i}]=$!;
i=${i+1};

echo "RUNNING FOR FUT2.. "
echo "$db_generate_exec ${PRODUCT_FILE_FUT}_1 $YYYYMMDD"
taskset -c 6,7 $db_generate_exec ${PRODUCT_FILE_FUT}_1 $YYYYMMDD
pids[${i}]=$!;
i=${i+1};

echo "RUNNING BHAV AND TECHINCAL INDICATORS.. "
echo "${db_bhav_exec} ${YYYYMMDD}"
taskset -c 4,5 ${db_bhav_exec} ${YYYYMMDD}

for pid in ${pids[*]}; do
    wait $pid
done
