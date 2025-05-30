#!/bin/bash

USAGE1="$0 YYYYMMDD SESSION"
EXAMP1="$0 20130725 NTAPROD1"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1 ;
SESSION=$2 ;

if [ "$YYYYMMDD" == "TODAY" ]
then 
   
    YYYYMMDD=$(date "+%Y%m%d") ;

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

TEMP_STATS_FILE=/tmp/ETI_MESSAGE_COUNT_$SESSION"_"$YYYYMMDD".dat" ;
FILE_SERVER=10.23.74.40
USER=dvcinfra
BACKUPLOC=/apps/data/MFGlobalTrades/ETIMessageCount/ ;

zgrep "Msg Count" "/spare/local/ORSlogs/EUREX/$SESSION/log.$YYYYMMDD".gz"" > $TEMP_STATS_FILE ;
gzip $TEMP_STATS_FILE ;

scp $TEMP_STATS_FILE".gz" $USER@$FILE_SERVER:$BACKUPLOC

rm -rf $TEMP_STATS_FILE".gz" ;
