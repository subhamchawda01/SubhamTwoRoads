#
#   file scripts/upload_MOV_files.sh 
#
#   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#   Address:
#   Suite No 162, Evoma, #14, Bhattarhalli,
#   Old Madras Road, Near Garden City College,
#   KR Puram, Bangalore 560049, India
#   +91 80 4190 3551


#!/bin/bash

USAGE1="$0 YYYYMMDD "
EXAMP1="$0 20120301 "

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=`date +"%Y%m%d"`
fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


HOST='ftp.linkinvestimentos.com.br';
USER='dvcapital';
PASSWD='6#A23nds2$';

LINKINVOICE_FILE="Link_InvoiceDetailed_35786_"$YYYYMMDD".csv";

mkdir -p /NAS1/data/MFGlobalTrades/MFGFiles 

cd /NAS1/data/MFGlobalTrades/MFGFiles;

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get $LINKINVOICE_FILE
quit
SCRIPT

chmod 666 $LINKINVOICE_FILE;

UPLOAD_FILE=/tmp/Link_Invoice_$YYYYMMDD".csv" ;

if [ -f $LINKINVOICE_FILE ]
then

    echo "LINK INVOICE FILE AVAILABLE & UPLOADING TO ALPES" | /bin/mail -s "Link Invoice File" -r "linkfileuploader@ny11" "nseall@tworoads.co.in";

else

    echo "LINK INVOICE FILE NOT AVAILABLE" | /bin/mail -s "Link Invoice File" -r "linkfileuploader@ny11" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in"

fi

cp $LINKINVOICE_FILE $UPLOAD_FILE ;

USER='dvcapital'
ALPES_SFTP='201.85.54.210'

export SSHPASS=Dvbrazil#!

#### SFTP #### fetch cash statements txt

sshpass -e sftp -oBatchMode=no -b - $USER@$ALPES_SFTP  << !

   put $UPLOAD_FILE
   bye

!

#############


rm $UPLOAD_FILE ;
