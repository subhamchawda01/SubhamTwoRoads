#!/bin/bash

# USAGE:
# 
# gunzips the logs saved to the directories in '/spare/local/logs/tradelogs/'
# Sends them over to dev ny4 in /apps/logs/QueryTrades/YYYY/MM/DD

#USAGE="$0    USER     YYYYMMDD     GZIP(Y/N)    CLEANUP(Y/N)";
USAGE="$0 YYYYMMDD=TODAY";
# USAGE_DESC="Gzips & syncs trading logs dated 'YYYYMMDD' to /apps/logs/QueryTrades/YYYY/MM/DD on USER @ ny4-server. Gzipping is controlled by GZIP (Y = Yes, N = No). CLEANUP (Y = Yes, N = No) will erase all trades files older than 15 days.";

if [ $# -ne 1 ] ;
then
    echo $USAGE;
#    echo $USAGE_DESC;
    exit;
fi

YYYYMMDD=$1;
#USER=$1;
#YYYYMMDD=$2;
#GZIP=$3;
#CLEANUP=$4;

DEST_SERVER="sdv-ny4-srv11";

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d");
fi

if [ $USER != "dvcinfra" ]
then
    echo "Script must be run as dvcinfra";
    exit 0;
fi

TRADES_FILES="/spare/local/ORSlogs/CME/*/trades."$YYYYMMDD".*";
DEST_TRD_LOC="/apps/logs/ORSTrades/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
DEST_LOG_LOC="/apps/logs/ORSLogs/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};

LOGFILES="/spare/local/ORSlogs/CME/*/log."$YYYYMMDD".*";
for name in $LOGFILES ;
do
    gzip $name ;
done
GZIPPED_LOGFILES="/spare/local/logs/tradelogs/log."$YYYYMMDD".*.gz";

# echo $DEST_TRD_LOC

# Create the directory if needed.
# echo ssh $DEST_SERVER -l $USER 'mkdir -p '$DEST_TRD_LOC;
ssh $DEST_SERVER -l $USER 'mkdir -p '$DEST_TRD_LOC;
scp -q $TRADES_FILES $USER@$DEST_SERVER:$DEST_TRD_LOC/;

ssh $DEST_SERVER -l $USER 'mkdir -p '$DEST_LOG_LOC;
scp -q $GZIPPED_LOGFILES $USER@$DEST_SERVER:$DEST_LOG_LOC/;

#if [ $CLEANUP = "Y" ] ;
#then
find /spare/local/logs/tradelogs -name log.\* -type f -mtime +15 -exec rm '{}' \; 
#fi
