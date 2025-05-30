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

# Get the needed files from the NewEdge ftp.
mkdir -p /apps/data/MFGlobalTrades/MFGFiles;
cd /apps/data/MFGlobalTrades/MFGFiles;

HOST='Ftp.newedgegroup.com';
USER='DV_CAPITAL@USA';
PASSWD='GNVVfbY8';

MNY_FILE="GMIMNY_"$YYYYMMDD".csv";
POS_FILE="GMIPOS_"$YYYYMMDD".csv";
IST_FILE="GMIST4_"$YYYYMMDD".csv";
TRN_FILE="GMITRN_"$YYYYMMDD".csv";

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get $MNY_FILE
get $POS_FILE
get $IST_FILE
get $TRN_FILE
quit
SCRIPT

#gzip all files
gzip -f *  #compresses removing the old file

rsync -avz /apps/data/MFGlobalTrades/MFGFiles/ dvcinfra@10.23.74.41:/apps/data/MFGlobalTrades/MFGFiles

#remove all files older than 5 days
OLD_FILES=(`find ./ -mtime +5`)
if [ ! -z $OLD_FILES ] ; then
for old_file in "${OLD_FILES[@]}"
do
    rm -f $old_file  #removing the old file
done
fi

