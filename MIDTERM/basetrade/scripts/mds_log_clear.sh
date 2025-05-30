#!/bin/bash

# USAGE:
# 
# Clears the logs for the specified exchange and the specified date.

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

MDSLOG_DIR="/spare/local/MDSlogs";

echo $0" : Moving into " $MDSLOG_DIR"/"$EXCHANGE

cd $MDSLOG_DIR
cd $EXCHANGE

echo $0" : erasing all logs for "$EXCHANGE" for "$YYYYMMDD
#rm -f *$YYYYMMDD Not doing this. Worried we'll lose data.

echo $0" : erasing all files for "$EXCHANGE" older than 5 days"
find -type f -mtime +5 -exec rm -f {} \;

echo $0" : CLEARDATA completed."