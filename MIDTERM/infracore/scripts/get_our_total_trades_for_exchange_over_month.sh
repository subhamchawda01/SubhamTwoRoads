#!/bin/bash

USAGE1="$0 EXCHANGE YYYYMM"
EXAMP1="$0 EUREX 201203 "

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

EXCH=$1
YYYYMM=$2

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


ORS_TRADES_DIR="/NAS1/logs/ORSTrades/$EXCH"
SUM_CALC_SCRIPT=$HOME/infracore/scripts/sumcalc.pl

TEMP_TRADE_FILE=/tmp/temp_trades_file.txt

PATERN_SEARCH="trades."$YYYYMM ;


>$TEMP_TRADE_FILE ;

for tradesfile in `find $ORS_TRADES_DIR -name "*$PATERN_SEARCH*"` 
do

    wc -l $tradesfile >> $TEMP_TRADE_FILE ;

done

cat $TEMP_TRADE_FILE | awk '{print $1}' | $SUM_CALC_SCRIPT ;

#trades file just generated in case details are required, otherwise direct sum is faster
rm -rf $TEMP_TRADE_FILE ;

