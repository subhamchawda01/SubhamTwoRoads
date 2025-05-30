#!/bin/bash

YYYYMMDD=`date +\%Y\%m\%d`
YYYY=${YYYYMMDD:0:4}
MM=${YYYYMMDD:4:2}

EXPIRY=$YYYYMMDD
echo "Expiry : $EXPIRY";
NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%Y"`
N_MM=${NEXT_MONTH:0:2}
N_YY=${NEXT_MONTH:4:2}
echo "NextMonth: $N_MM , $N_YY"

[[ -f "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${N_MM}${N_YY}.csv" ]] && { echo "FIle exists ";exit; }

/home/dvctrader/important/onexpiry/fetch_nse_on_expiry_part1.sh

[[ $? != 0  ]] && exit
[[ ! -f "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${N_MM}${N_YY}.csv" ]] && { echo "FIle Not exists ";exit; }


/home/dvctrader/important/onexpiry/fetch_nse_on_expiry_part2.sh $YYYYMMDD

/home/dvctrader/important/onexpiry/fetch_nse_on_expiry_part3.sh $YYYYMMDD

echo ""| mailx -s "File updated On Expiry" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in

#sync all trade info to the prod machines
#/home/dvctrader/important/onexpiry/sync_trade_info.sh

