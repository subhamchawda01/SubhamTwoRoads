#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Gets rencap files from FTP.";

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
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

# Get the needed files from the NewEdge ftp.
mkdir -p /apps/data/RencapTrades/RencapFiles;
cd /apps/data/RencapTrades/RencapFiles;

RTS_file="D"$YYYYMMDD"_DCF491_1.xls"
MICEX_file="D"$YYYYMMDD"_DMS491_1.xls"
DMC_file="D"$YYYYMMDD"_DMC491_1.xls" 	#contains no data for now

HOST='66.185.19.8';
USER='DVCapital';
PASSWD='8aKsodBQ';

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get $RTS_file
get $MICEX_file
get $DMC_file
quit
SCRIPT

#gzip all files
gzip -f *  #compresses removing the old file

rsync -avz /apps/data/RencapTrades/RencapFiles/ dvcinfra@10.23.74.41:/apps/data/RencapTrades/RencapFiles

#remove all files older than 5 days
OLD_FILES=(`find ./ -mtime +5`)
if [ ! -z $OLD_FILES ] ; then
for old_file in "${OLD_FILES[@]}"
do
    rm -f $old_file  #removing the old file
done
fi

