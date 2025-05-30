#!/bin/bash

# USAGE:
# 
# tars & gunzips the logs saved to the directories in '/spare/local/MDSlogs/'
# Sends them over to the specified exchange.

USAGE="$0    EXCHANGE    CURRENT_LOCATION    YYYYMMDD";
USAGE_DESC="Compresses logs for exchange 'EXCHANGE' dated 'YYYYMMDD' at 'CURRENT_LOCATION' server.";

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

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi



MDSLOG_DIR="/spare/local/MDSlogs";

echo $0" : Moving into " $MDSLOG_DIR"/"$EXCHANGE

cd $MDSLOG_DIR/$EXCHANGE

# Archiving the logs.
ARCH_NAME=$EXCHANGE$CURRENT_LOCATION$YYYYMMDD'.tar'

echo $0" : Archiving logs "
gzip  *$YYYYMMDD

