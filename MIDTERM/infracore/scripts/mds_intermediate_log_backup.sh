#!/bin/bash

# USAGE:
# 
# tars & gunzips the logs saved to the directories in '/spare/local/MDSlogs/'
# Sends them over to the specified exchange.

USAGE="$0    EXCHANGE    CURRENT_LOCATION    YYYYMMDD";
USAGE_DESC="Syncs logs for exchange 'EXCHANGE' dated 'YYYYMMDD' to 'CURRENT_LOCATION' server.";

if [ $# -ne 3 ] ;
then
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

DEST_USER="dvcinfra"

EXCHANGE=$1;
CURRENT_LOCATION=$2;
YYYYMMDD=$3;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


DO_NOT_COPY_LOGS_FILE=/spare/local/MDSlogs/"DONT_COPY_"$EXCHANGE

if [ $YYYYMMDD = "TODAY" ] ;
then

    #if the DO_NOT_COPY file was modified in last 20 hrs don't copy data
    if [ `find $DO_NOT_COPY_LOGS_FILE -mmin -1200 -print 2>/dev/null | wc -l` == 1 ]
    then
        echo "Data Logged For "$EXCHANGE" At "$CURRENT_LOCATION" Is Not To Be Copied Over"
        echo "Remove File "$DO_NOT_COPY_LOGS_FILE" To Copy Forcefully"
        exit;
    fi

    YYYYMMDD=$(date "+%Y%m%d")
fi

DEST_SERVER="10.23.74.40"

MDSLOG_DIR="/spare/local/MDSlogs";

MDSLOGS_COPY_TEMP_DIR=/tmp/MDS_LOG_COPY_TEMP_DIR/ ;

mkdir -p $MDSLOGS_COPY_TEMP_DIR ;

echo $0" : Moving into " $MDSLOG_DIR"/"$EXCHANGE

cd $MDSLOG_DIR/$EXCHANGE

# Archiving the logs.
ARCH_NAME=$EXCHANGE$CURRENT_LOCATION$YYYYMMDD'.tar'

echo $0" : Archiving logs to "$ARCH_NAME

cp *$YYYYMMDD $MDSLOGS_COPY_TEMP_DIR/ 
cd $MDSLOGS_COPY_TEMP_DIR ;

gzip  *$YYYYMMDD

tar -cvf $ARCH_NAME *$YYYYMMDD.gz

# Store the return code for error checking.
echo $? > /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : Sending logs to "$DEST_SERVER

# Must add RSA public key for this to work without prompting for a password.

rsync -avz --bwlimit 1280 $ARCH_NAME $DEST_USER@$DEST_SERVER:/apps/data/

###scp $ARCH_NAME $DEST_USER@$DEST_SERVER:/NAS1/data/

# Store the return code for error checking.
echo $? >> /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : Connecting to "$DEST_SERVER
# Must add RSA public key for this to work without prompting for a password.
# Run local script on remote server.
# note the mds_unpack path...
ssh $DEST_USER@$DEST_SERVER 'bash -s' < ~/LiveExec/scripts/mds_log_unpack.sh $EXCHANGE $CURRENT_LOCATION $YYYYMMDD

# Store the return code for error checking.
echo $? >> /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : erasing "$ARCH_NAME
rm $ARCH_NAME

# Find if there was an error
ERRORCODE=$(grep -v "0" /tmp/$EXCHANGE"_"$YYYYMMDD | wc -l)

if [ $ERRORCODE != "0" ] ;
then
    echo $0" : ERROR => Copy FAILED."
    exit;
fi

echo $0" : SUCCESS => Copy completed."

# Only delete this file if copy succeeded.
rm /tmp/$EXCHANGE"_"$YYYYMMDD

rm -rf $MDSLOGS_COPY_TEMP_DIR ;
