#!/bin/bash

USAGE1="$0 SESSION DATE"
EXAMP1="$0 HC0 TODAY"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

SESSION=$1
YYYYMMDD=$2
ORSDIR=/spare/local/ORSlogs/ICE ;

TEMP_FILE=/tmp/exchange_sequence_temp.dat

DEST_USER=dvcinfra
DEST_HOST=10.23.74.51
DEST_DIR=/apps/data/MFGlobalTrades/AUDIT/FIX_LOGS/ICE

if [ $YYYYMMDD = "TODAY" ]
then

    YYYYMMDD=`date +"%Y%m%d"`

fi

LOGFILE=$ORSDIR/$SESSION/audit.$YYYYMMDD.*;


if [ X"$LOGFILE" = X ];
then

    echo $SESSION" LOGFILE NOT AVAILABLE" $LOGFILE | /bin/mail -s "AUDIT LOGS" -r "ExchSeqExtractor" "nseall@tworoads.co.in" ;
#    echo $SESSION" LOGFILE NOT AVAILABLE" $LOGFILE ;

fi

#cat $LOGFILE | sed 's/\(.\)\(....\)N2N/\1\n\2N2N/g' > $TEMPSEQFILE ;

#cat $TEMPSEQFILE | awk '{print $2 " " $3 " " $4 " " $1}' | sed 's/://g' > $TEMP_FILE ;
ssh $DEST_USER"@"$DEST_HOST "mkdir -p $DEST_DIR";
scp $LOGFILE $DEST_USER"@"$DEST_HOST":"$DEST_DIR/ ;

#rm -rf $TEMP_FILE ;

