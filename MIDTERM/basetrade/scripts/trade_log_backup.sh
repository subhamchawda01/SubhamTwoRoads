#!/bin/bash
USAGE="$0    EXCHANGE    PROFILE   YYYYMMDD";

if [ $# -ne 3 ] ;
then
    echo $USAGE
    exit;
fi

USER="dvcinfra"

EXCHANGE=$1;
PROFILE=$2;
YYYYMMDD=$3;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

DEST_SERVER="sdv-ny4-srv11"

TRADELOG_DIR="/spare/local/ORSlogs";
TRADEFILE=$TRADELOG_DIR"/"$EXCHANGE"/"$PROFILE"/"trades.$YYYYMMDD; 

if [ -e $TRADEFILE ] ; then

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2} 
ssh $DEST_SERVER -l $USER "mkdir -p /apps/logs/ORSTrades/$EXCHANGE/$PROFILE/$NEWDIR"
scp -q $TRADEFILE $USER@$DEST_SERVER:/apps/logs/ORSTrades/$EXCHANGE/$PROFILE/$NEWDIR/
fi
