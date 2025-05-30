#!/bin/bash



# all_volumes_on_day CME 20110912

USAGE1="$0 EXCH YYYYMMDD "
EXAMP1="$0 CME 20110912"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

EXEC=$HOME/LiveExec/bin/all_live_volumes_on_day_from_generic_data

EXCH=$1;
YYYYMMDD=$2;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

DIRLOC=/spare/local/MDSlogs/GENERIC

for files in `ls $DIRLOC | grep "$YYYYMMDD"`
do

   $EXEC $EXCH $DIRLOC/$files ;

done 
