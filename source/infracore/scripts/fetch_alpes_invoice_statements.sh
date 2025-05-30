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

    YYYYMMDD=`ssh dvcinfra@10.23.74.51 'cat /tmp/YESTERDAY_DATE'` ;

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


USER='dvcapital'
ALPES_SFTP='201.85.54.210'

STATEMENT_FILE="InvoiceDetailed_4505_"$YYYYMMDD".csv" ;

OUT_DIR=/apps/data/MFGlobalTrades/MFGFiles/
cd $HOME

export SSHPASS=Dvbrazil#!

#### SFTP #### fetch cash statements txt

sshpass -e sftp -oBatchMode=no -b - $USER@$ALPES_SFTP  << !

   get *$YYYYMMDD*
   bye

!

#############

if [ -f $STATEMENT_FILE ] 
then

    scp $STATEMENT_FILE dvcinfra@10.23.74.51:$OUT_DIR 

else 

    echo "NOT AVAILABLE" ;
    echo "ALPES INVOICE FILE NOT AVAILABLE" | /bin/mail -s "Alpes Invoice File" -r "alpesinvoice@ny11" "nseall@tworoads.co.in" ;

fi     
