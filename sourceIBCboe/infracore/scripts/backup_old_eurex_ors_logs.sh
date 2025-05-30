#!/bin/bash
USAGE="$0 GZIPPED_EUREX_ORS_LOGFILE";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

GZIPPED_LOGFILE=$1;

EXCHANGE=EUREX;
YYYYMMDD=`echo $GZIPPED_LOGFILE | cut -c 39-46`;
PROFILE=`echo $GZIPPED_LOGFILE | cut -c 28-33` ;


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

if [ $USER != "dvcinfra" ]
then
    echo "Script must be run as dvcinfra";
    exit 0;
fi

DEST_SERVER="10.23.74.40"
NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2} 

TRADELOG_DIR="/spare/local/ORSlogs";
cd $TRADELOG_DIR"/"$EXCHANGE"/"$PROFILE;

# TRADEFILE=$TRADELOG_DIR"/"$EXCHANGE"/"$PROFILE"/"trades.$YYYYMMDD; 
# DESTTRADEDIR=/apps/logs/ORSTrades;
# if [ -e $TRADEFILE ] ; then
#     ssh $DEST_SERVER -l $USER "mkdir -p $DESTTRADEDIR/$EXCHANGE/$PROFILE/$NEWDIR"
#     rsync -avz --quiet $TRADEFILE $USER@$DEST_SERVER:$DESTTRADEDIR/$EXCHANGE/$PROFILE/$NEWDIR/
# fi

DESTLOGDIR=/apps/logs/ORSLogs;
LOGFILE=$TRADELOG_DIR"/"$EXCHANGE"/"$PROFILE"/"log.$YYYYMMDD; 
if [ -e $LOGFILE ] ; then
    gzip $LOGFILE;
fi
#GZIPPED_LOGFILE=$LOGFILE".gz";#already argument
if [ -e $GZIPPED_LOGFILE ] ; then 
    ssh $DEST_SERVER -l $USER "mkdir -p $DESTLOGDIR/$EXCHANGE/$PROFILE/$NEWDIR"
    rsync -avz --quiet $GZIPPED_LOGFILE $USER@$DEST_SERVER:$DESTLOGDIR/$EXCHANGE/$PROFILE/$NEWDIR/
fi

