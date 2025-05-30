#!/bin/bash
#   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#   Address:
#   Suite No 162, Evoma, #14, Bhattarhalli,
#   Old Madras Road, Near Garden City College,
#   KR Puram, Bangalore 560049, India
#   +91 80 4190 3551

USAGE1="$0 YYYYMMDD "
EXAMP1="$0 20120301 "

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;

if [ $YYYYMMDD = "YESTERDAY" ] ;
then

    YYYYMMDD=`ssh dvcinfra@10.23.74.51 'cat /tmp/YESTERDAY_DATE'` ;

fi


USER='dv' ;
SFTP='sftp.brasilplural.com' ;
PORT=322
export SSHPASS=DV#plu2015! ;


# fetch
mkdir -p $HOME/plural_files/ ;
cd $HOME/plural_files/ ;

echo "sshpass -e sftp -P $PORT -oBatchMode=no -b - $USER@$SFTP  << !"
sftp -oPort=$PORT -oBatchMode=no -b - $USER@$SFTP  << !

   get dv/*$YYYYMMDD*
   bye
!

#transfer
OUT_DIR=/apps/data/MFGlobalTrades/MFGFiles/
rsync -avpz $HOME/plural_files/*$YYYYMMDD* dvcinfra@10.23.74.40:$OUT_DIR/

#clean
cd $HOME/plural_files/ ;
rm -rf *$YYYYMMDD* ;
