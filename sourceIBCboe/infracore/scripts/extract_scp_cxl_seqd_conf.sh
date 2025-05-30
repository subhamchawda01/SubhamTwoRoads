#!/bin/bash

USAGE1="$0 SHORTCODE DATE ORSDIR"
EXAMP1="$0 ZN_0 TODAY /spare/local/ORSlogs/CME/4AK"

if [ $# -ne 3 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

SHORTCODE=$1
YYYYMMDD=$2
ORSDIR=$3

SHORTCODE_SYMBOL_EXEC=$HOME/LiveExec/bin/get_exchange_symbol
DEST_SRV="10.23.74.40"
TRGT_LOC=/NAS1/data/ORSData/CXLCONF/

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ $YYYYMMDD = "TODAY" ]
then

    YYYYMMDD=`date +"%Y%m%d"`

fi

SYMBOL=`$SHORTCODE_SYMBOL_EXEC $SHORTCODE $YYYYMMDD`

TEMP_FILE=/tmp/$SYMBOL"_"$YYYYMMDD
touch $TEMP_FILE
>$TEMP_FILE

strings $ORSDIR/"log."$YYYYMMDD | egrep "CXL Send|CXL Conf" | awk '{print $2" "$3" "$4}' > $TEMP_FILE

scp $TEMP_FILE $DEST_SRV":"$TRGT_LOC

rm -rf $TEMP_FILE
