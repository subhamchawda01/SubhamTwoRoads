#!/bin/bash

get_expiry_date () {
  ONE=01;
  EXPIRY=$YYYYMMDD;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}


if [ $# -ne 1 ] ; then
  echo "Called As : script DATE" ;
  exit
fi
echo "--------------------------------------------- CHECKING EXPIRY -------------------------------------"

YYYYMMDD=$1
YYYY=${YYYYMMDD:0:4}
MM=${YYYYMMDD:4:2}
get_expiry_date;
echo "Expiry : $EXPIRY";
if [[ $EXPIRY != $YYYYMMDD ]];then
    echo "---------------------------------NOT EXPIRY $EXPIRY-------------------------------------------"
    echo 
    exit
fi
echo "---------------------------------TODAY IS EXPIRY $EXPIRY-------------------------------------------"
echo "---------------------------------RUNNING FETCH--------------------------------"
echo 

NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%Y"`
N_MM=${NEXT_MONTH:0:2}
N_YY=${NEXT_MONTH:4:2}
echo "NextMonth: $N_MM , $N_YY"

[[ -f "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${N_MM}${N_YY}.csv" ]] && { echo "FIle exists ";exit; }

/home/dvctrader/important/onexpiry/fetch_nse_on_expiry_part1.sh

[[ $? != 0  ]] && { echo "ERROR SCRIPT1"; exit; }
[[ ! -f "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${N_MM}${N_YY}.csv" ]] && { echo "FIle Not exists ";exit; }


/home/dvctrader/important/onexpiry/fetch_nse_on_expiry_part2.sh $YYYYMMDD

/home/dvctrader/important/onexpiry/fetch_nse_on_expiry_part3.sh $YYYYMMDD

echo ""| mailx -s "File updated On Expiry" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in

#sync all trade info to the prod machines
#/home/dvctrader/important/onexpiry/sync_trade_info.sh

