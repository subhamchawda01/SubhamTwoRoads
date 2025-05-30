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


USER='1079431_000001'
HOST='193.29.90.129'
PORT=2221

OUT_DIR=/logs2/apps/logs/ExchangeFiles/EUREX/ ;
mkdir -p $HOME/eurex_files/$YYYYMMDD ;
cd $HOME/eurex_files/$YYYYMMDD ;

KEY=/home/dvcinfra/.ssh/eurex_key
FILE=
#### SFTP #### fetch cash statements txt


sftp -b - -oPort=$PORT  -oIdentityFile=$KEY $USER@$HOST << !
get publi/P/eurex/$YYYYMMDD/*TA111*
bye

!
#############

chmod 755 * ;

cd $HOME/eurex_files/ ;
rsync -avpz $YYYYMMDD dvcinfra@10.23.74.41:$OUT_DIR ;

#rm -rf $YYYYMMDD ;

