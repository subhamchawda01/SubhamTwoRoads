#!/bin/bash


USAGE1="$0 YYYYMMDD "
EXAMP1="$0 20120301 "

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;

if [ $YYYYMMDD == "YESTERDAY" ] ;
then
    YYYYMMDD=`ssh dvcinfra@10.23.74.51 'cat /tmp/YESTERDAY_DATE'` ;
fi

mkdir -p /apps/data/MFGlobalTrades/MFGFiles;
cd /apps/data/MFGlobalTrades/MFGFiles;

HOST='Ftp.newedgegroup.com';
USER='DV_CAPITAL@USA';
PASSWD='GNVVfbY8';

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get *$YYYYMMDD*
quit
SCRIPT

cd /apps/data/MFGlobalTrades/MFGFiles;

chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*$YYYYMMDD*.csv ;
chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*.txt ;

OUT_DIR=/logs2/apps/logs/ExchangeFiles/EUREX/ ;
rsync -avpz /apps/data/MFGlobalTrades/MFGFiles dvcinfra@10.23.74.41:$OUT_DIR ;
