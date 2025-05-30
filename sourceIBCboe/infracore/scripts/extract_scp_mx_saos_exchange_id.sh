#!/bin/bash

USAGE1="$0 SESSION DATE"
EXAMP1="$0 BDMA TODAY"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

SESSION=$1
YYYYMMDD=$2
ORSDIR=/spare/local/ORSlogs/TMX ;

TEMP_FILE=/tmp/exchange_sequence_temp.dat 

DEST_USER=dvcinfra
DEST_HOST=10.23.74.51
DEST_DIR=/apps/data/MFGlobalTrades/AUDIT/EXCHANGE_SEQ

if [ $YYYYMMDD = "TODAY" ]
then

    YYYYMMDD=`date +"%Y%m%d"`

fi

LOGFILE=$ORSDIR/$SESSION/AUDIT_$YYYYMMDD

if [ ! -f $LOGFILE ] 
then 

    echo $SESSION" LOGFILE NOT AVAILABLE" $LOGFILE | /bin/mail -s "AUDIT LOGS" -r "ExchSeqExtractor" "nseall@tworoads.co.in" ;

fi

cat $LOGFILE | awk '{print $2 " " $3 " " $5 " " $1}'  > $TEMP_FILE ;

scp $TEMP_FILE $DEST_USER"@"$DEST_HOST":"$DEST_DIR"/"$SESSION"."$YYYYMMDD".dat" ;

rm -rf $TEMP_FILE ;
rm -rf /tmp/log.$YYYYMMDD

