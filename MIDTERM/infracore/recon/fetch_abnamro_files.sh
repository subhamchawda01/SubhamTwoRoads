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
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

# Get the needed files from the abnamro ftp.
mkdir -p /apps/data/AbnamroTrades/AbnamroFiles;
cd /apps/data/AbnamroTrades/AbnamroFiles;

USER='2307' ;
ABNAMRO_SFTP='91.213.201.123' ;

# fetch
sftp -oBatchMode=no -b - $USER@$ABNAMRO_SFTP  << SCRIPT
   get outgoing/$YYYYMMDD*TRX*csv.zip
   bye
SCRIPT

GENERAL_TRD_FILE="/apps/data/AbnamroTrades/AbnamroFiles/"$YYYYMMDD"_abnamro_trx.csv"
>$GENERAL_TRD_FILE

FILES=$(find ./ -name "$YYYYMMDD*TRX*csv.zip" | tr ' ' '~')
if [ ! -z "$FILES" ] ; then
	filename=`echo $FILES | tr '~' ' '`
	unzip -o "$filename"
	rm "$filename" 
fi

FILES=$(find ./ -name "$YYYYMMDD*TRX*csv" | tr ' ' '~')
if [ ! -z "$FILES" ] ; then
	filename=`echo $FILES | tr '~' ' '`
	mv "$filename" $GENERAL_TRD_FILE
fi

#gzip all files
gzip -f *  #compresses removing the old file

rsync -avz /apps/data/AbnamroTrades/AbnamroFiles/ dvcinfra@10.23.74.41:/apps/data/AbnamroTrades/AbnamroFiles

#remove all files older than 5 days
OLD_FILES=(`find ./ -mtime +5`)
if [ ! -z $OLD_FILES ] ; then
for old_file in "${OLD_FILES[@]}"
do
    rm -f $old_file  #removing the old file
done
fi
