#!/bin/bash

# USAGE:
# 
# Clears the logs for the specified exchange and the specified date.
# Taken from 

USAGE="$0    EXCHANGE    YYYYMMDD";
USAGE_DESC="Clears logs for exchange 'EXCHANGE' dated 'YYYYMMDD'.";

if [ $# -ne 2 ];
then
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

EXCHANGE=$1;
YYYYMMDD=$2;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

ERRORFILE=/tmp/$EXCHANGE"_"$YYYYMMDD;

if [ -f $ERRORFILE ] ;
then
    # Existence of this file means that backup failed.
    # DO NOT DELETE THE DATA FILES.
    echo $0" : CLEARDATA FAILED because COPYDATA FAILED. Run COPYDATA again and retry CLEARDATA."
    exit;
fi

ORSBCASTLOG_DIR="/spare/local/ORSBCAST";

echo $0" : Moving into " $ORSBCASTLOG_DIR"/"$EXCHANGE

cd $ORSBCASTLOG_DIR
cd $EXCHANGE

echo $0" : erasing all logs for "$EXCHANGE" for "$YYYYMMDD
rm -f *$YYYYMMDD

echo $0" : CLEARDATA completed."
