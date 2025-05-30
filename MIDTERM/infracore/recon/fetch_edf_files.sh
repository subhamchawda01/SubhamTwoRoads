#!/bin/bash

USAGE="$0 YYYYMMDD \n";

if [ $# -ne 1 ] ;
then
    echo -e $USAGE;
    exit;
fi

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=`date --date='yesterday' +"%Y%m%d"` ;
fi

mkdir -p /apps/data/EDFTrades/EDFFiles;
cd /apps/data/EDFTrades/EDFFiles;

#sftp -oPort=2222 -oBatchMode=no DVCAPITAL@ftpc.edfmancapital.com
HOST='ftpc.edfmancapital.com';
USER='DVCAPITAL';
PORT="2222"
PASSWD='SideChairFixSeven';

IST_FILE="PFDFST4_"$YYYYMMDD".CSV"
#connect to ftp and fetch files
sftp -oPort="$PORT" -oBatchMode=no -b - $USER@$HOST  << SCRIPT
   get "$IST_FILE"
   bye
SCRIPT

#gzip all files
gzip -f *  #compresses removing the old file

rsync -avz /apps/data/EDFTrades/EDFFiles/ dvcinfra@10.23.74.41:/apps/data/EDFTrades/EDFFiles

#remove all files older than 5 days
OLD_FILES=(`find ./ -mtime +5 `)
if [ ! -z $OLD_FILES ] ; then
for old_file in "${OLD_FILES[@]}"
do
    rm -f $old_file  #removing the old file
done
fi