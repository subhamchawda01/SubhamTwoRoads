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

USER="dvcinfra"

EXCHANGE=$1;
CURRENT_LOCATION=$2;
YYYYMMDD=$3;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

#sdv-ny4-srv11 = 10.1.3.11
DEST_SERVER="10.1.3.11"

MDSLOG_DIR="/spare/local/MDSlogs";

echo $0" : Moving into " $MDSLOG_DIR"/"$EXCHANGE

cd $MDSLOG_DIR/$EXCHANGE

# Archiving the logs.
ARCH_NAME=$EXCHANGE$CURRENT_LOCATION$YYYYMMDD'.tar'

echo $0" : Archiving logs to "$ARCH_NAME
gzip  *$YYYYMMDD

tar -cvf $ARCH_NAME *$YYYYMMDD.gz

# Store the return code for error checking.
echo $? > /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : Sending logs to "$DEST_SERVER

# Must add RSA public key for this to work without prompting for a password.
scp $ARCH_NAME $USER@$DEST_SERVER:/NAS1/data/

# Store the return code for error checking.
echo $? >> /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : Connecting to "$DEST_SERVER
# Must add RSA public key for this to work without prompting for a password.
# Run local script on remote server.
# note the mds_unpack path...
ssh $USER@$DEST_SERVER 'bash -s' < ~/LiveExec/scripts/mds_log_unpack.sh $EXCHANGE $CURRENT_LOCATION $YYYYMMDD

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
