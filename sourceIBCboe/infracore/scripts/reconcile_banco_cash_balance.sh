#!/bin/bash

#
#   file scripts/reconcile_banco_cash_balance.sh
#
#   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#   Address:
#   Suite No 162, Evoma, #14, Bhattarhalli,
#   Old Madras Road, Near Garden City College,
#   KR Puram, Bangalore 560049, India
#   +91 80 4190 3551

USAGE1="$0 SETTLEMENT_DATE( YYYYMMDD )"
EXAMP1="$0 YESTERDAY"

if [ $# -ne 1 ] ;
then

    echo $USAGE1;
    echo $EXAMP1;
    exit;

fi

YYYYMMDD=$1

if [ $YYYYMMDD = "YESTERDAY" ]
then

    ### Settlement Date 
    YYYYMMDD=`date +"%Y%m%d" -d "yesterday"`  

fi

## Trading Date previous pnls to reconcile in trading date's cash journals & carry forward reserves 
twodaysagoday=`date +"%A" -d "2 days ago"`
TwoDaysAgoDate=`date +"%Y%m%d" -d "2 days ago"`

if [ $twodaysagoday = "Sunday" ]
then
   
   ## move back to friday, not considering exchange holidays right now
   TwoDaysAgoDate=`date +"%Y%m%d" -d "4 days ago"`

fi

ALLOWED_DIFF=100

#===================== BANCO Statements =====================  

CASH_STAT_DIR=/NAS1/data/MFGlobalTrades/MFGFiles
##@FILE_INFO : This file has the consolidated cash balance in reals - won't reflect the change for the trading date
##rather will show carry forward for the day prior than the trading date
SETTLE_DATE_CASH_BALANCE_FILE=$CASH_STAT_DIR/Statement_NRE000000043_$YYYYMMDD.txt 
TRADE_DATE_CASH_BALANCE_FILE=$CASH_STAT_DIR/Statement_NRE000000043_$TwoDaysAgoDate.txt

##@FILE_INFO : This would have the pnls for the trading date (2days ago date)
PNL_STAT_FILE=$CASH_STAT_DIR/Cash_Journal_NRE000000043_$YYYYMMDD.txt

BANCO_PNL=`cat $PNL_STAT_FILE | head -2 | tail -1 | awk '{print $7}'`

BANCO_SETTLE_DATE_CASH_BALANCE=`cat $SETTLE_DATE_CASH_BALANCE_FILE | head -2 | tail -1 | awk '{print $5}'`
BANCO_TRADE_DATE_CASH_BALANCE=`cat $TRADE_DATE_CASH_BALANCE_FILE | head -2 | tail -1 | awk '{print $5}'`

FWD_BAL=`echo $BANCO_SETTLE_DATE_CASH_BALANCE " " $BANCO_TRADE_DATE_CASH_BALANCE_ | awk '{print ($1-$2)}'`

#=======================================================


#===================== OUR PNLS (LINK)=====================  

EXCH_PNL_DIR=/NAS1/data/MFGlobalTrades/ExchangePnl
EXCH_PNL_FILE=$EXCH_PNL_DIR/${TwoDaysAgoDate:0:4}/${TwoDaysAgoDate:4:2}/${TwoDaysAgoDate:6:2}/pnl.csv
BMF_PNL=`grep "BMF" $EXCH_PNL_FILE | tr ',' ' ' | awk '{print $3}'`

#=======================================================

### Currency Conversion ##

CURRENCY_RATES_DIR=/spare/local/files/CurrencyData
CURRENCY_RATES_FILE=$CURRENCY_RATES_DIR/currency_rates_$TwoDaysAgoDate.txt

USDBRL=`grep "USDBRL" $CURRENCY_RATES_FILE | awk '{print $2}'`

BANCO_PNL=`echo $BANCO_PNL | awk '{print $1/'$USDBRL'}'`
BANCO_SETTLE_DATE_CASH_BALANCE=`echo $BANCO_SETTLE_DATE_CASH_BALANCE | awk '{print $1/'$USDBRL'}'`
BANCO_TRADE_DATE_CASH_BALANCE=`echo $BANCO_TRADE_DATE_CASH_BALANCE | awk '{print $1/'$USDBRL'}'`
FWD_CAL=`echo $FWD_BAL | awk '{print $1/'$USDBRL'}'`

#=======================================================

#===================== MAIL FILE===========================

MAIL_FILE=/tmp/banco_cash_balance_reconcliation.txt
touch $MAIL_FILE
>$MAIL_FILE

#=======================================================


#==================Reconciliation====================

# absoulute pnl diff
PNL_DIFF=`echo $BMF_PNL " " $BANCO_PNL | awk '{print ($1-$2)}' | awk '{if( $1>0 ) {print $1} else {print 0-$1}}'`

BAL_DIFF=`echo $BANCO_SETTLE_DATE_CASH_BALANCE " " $BANCO_TRADE_DATE_CASH_BALANCE | awk '{print ($1-$2)}'`

# absolute fwd cash difference
FWD_CASH_DIFF=`echo $BAL_DIFF " " $BMF_PNL | awk '{print ($1-$2)}' | awk '{ if( $1>0) {print $1} else {print 0-$1}}'`

echo "Trading Date : " $TwoDaysAgoDate " & Settlement Date : " $YYYYMMDD  >> $MAIL_FILE
echo >> $MAIL_FILE

echo $PNL_DIFF | awk '{ if( $1 > '$ALLOWED_DIFF' ) {print "PNL Discrepancy. Our PNL : '$BMF_PNL' Banco PNL : '$BANCO_PNL' \tAbs PNL Diff : " $1 } else { print " PNL Match : '$BANCO_PNL' " } }' >> $MAIL_FILE

echo $FWD_CASH_DIFF | awk '{ if( $1 > '$ALLOWED_DIFF' ) {print "Cash Balance Discrepancy. Previous Bal : '$BANCO_TRADE_DATE_CASH_BALANCE' Closing Bal : '$BANCO_SETTLE_DATE_CASH_BALANCE' \tAbs Balance Diff : '$FWD_CASH_DIFF' "} else { print " Balance Match. Closing Balance : '$BANCO_SETTLE_DATE_CASH_BALANCE' " } }' >> $MAIL_FILE


#=================================================

/bin/mail -s "Banco Cash Balance" -r "cashbalance.info@ny11" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in gchak@circulumvite.com rakesh.kumar@tworoads.co.in" 

rm -rf $MAIL_FILE
