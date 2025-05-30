#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

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

# Get the needed files from the plural ftp.
mkdir -p /apps/data/PluralTrades/PluralFiles;
cd /apps/data/PluralTrades/PluralFiles;

USER='dv' ;
BCS_SFTP='sftp.brasilplural.com' ;
PORT=322
export SSHPASS=DV#plu2015! ;


# fetch
sshpass -e sftp -oPort=$PORT -oBatchMode=no -b - $USER@$BCS_SFTP  << SCRIPT
   get dv/*$YYYYMMDD*
   bye
SCRIPT

#gzip all files
gzip -f *  #compresses removing the old file

rsync -avz /apps/data/PluralTrades/PluralFiles/ dvcinfra@10.23.74.41:/apps/data/PluralTrades/PluralFiles

#remove all files older than 5 days
OLD_FILES=(`find ./ -mtime +5`)
if [ ! -z $OLD_FILES ] ; then
for old_file in "${OLD_FILES[@]}"
do
    rm -f $old_file  #removing the old file
done
fi

