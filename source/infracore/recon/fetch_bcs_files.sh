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

# Get the needed files from the bcs ftp.
mkdir -p /apps/data/BCSTrades/BCSFiles;
cd /apps/data/BCSTrades/BCSFiles;

USER='dvcapital' ;
BCS_SFTP='89.249.27.193' ;
PORT=2222
export SSHPASS=QhWmPc ;

# fetch
sshpass -e sftp -oPort=$PORT -oBatchMode=no -b - $USER@$BCS_SFTP  << SCRIPT

   get public/*/*$YYYYMMDD*zip
   bye
SCRIPT

GENERAL_TRD_FILE="/apps/data/BCSTrades/BCSFiles/"$YYYYMMDD"_trades";

>$GENERAL_TRD_FILE

FILES=(`find ./ -name "*$YYYYMMDD*zip" `)
if [ ! -z $FILES ] ; then
for i in "${FILES[@]}"
do
    unzip -o $i
    rm $i
done
fi

FILES=(`find ./ -name "*$YYYYMMDD*csv" `)
if [ ! -z "$FILES" ] ; then
for i in "${FILES[@]}"
do
    cat $i >> $GENERAL_TRD_FILE
done
fi

OLD_FILES=(`find ./ -mtime +2 | grep -v "gz"`)
if [ ! -z $OLD_FILES ] ; then
for old_file in "${OLD_FILES[@]}"
do
    gzip -f $old_file  #compresses removing the old file
done
fi

COMPRESSED_FILES=(`find ./ -name "*gz"`)
if [ ! -z $COMPRESSED_FILES ] ; then
for compressed_file in "${COMPRESSED_FILES[@]}"
do
	scp $compressed_file dvcinfra@10.23.74.41:/apps/data/BCSTrades/BCSFiles/
    rm $compressed_file
done
fi
