#!/bin/bash

#
#   file scripts/fetch_link_cash_statements.sh
#
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
    YYYYMMDD=`cat /tmp/YESTERDAY_DATE`
fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


RSA_KEY=/home/dvcinfra/BMF_BANCO/bmf_banco_sftp.key
USER='dvcapitalllc_sftp'
BANCO_SFTP='knox.bmfbovespa.com.br'

OUT_DIR=/NAS1/data/MFGlobalTrades/MFGFiles
cd $OUT_DIR

#### SFTP #### fetch cash statements txt

sftp -b - -oPort=20681 -oIdentityFile=$RSA_KEY $USER@$BANCO_SFTP << !

cd /in/private/TXT
get *$YYYYMMDD*        
bye

!

#############
