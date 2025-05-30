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


cd /NAS1/data/MFGlobalTrades/MFGFiles;

MOVFUT_FILE="MOVFUT_35786_"${YYYYMMDD:4:2}"_"${YYYYMMDD:6:2}"_"${YYYYMMDD:0:4}".txt";

HOST='ftp.linkinvestimentos.com.br';
USER='dvcapital';
PASSWD='6#A23nds2$';

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get $MOVFUT_FILE
quit
SCRIPT

RSA_KEY=/home/dvcinfra/BMF_BANCO/bmf_banco_sftp.key
USER='dvcapitalllc_sftp'
BANCO_SFTP='knox.bmfbovespa.com.br'

MOV_DIR=/NAS1/data/MFGlobalTrades/MFGFiles
MOV_FILE="MOVFUT_"${YYYYMMDD:4:2}""${YYYYMMDD:6:2}""${YYYYMMDD:0:4}"_35786.txt";
MOV_FILE_IN="MOVFUT_35786_"${YYYYMMDD:4:2}"_"${YYYYMMDD:6:2}"_"${YYYYMMDD:0:4}".txt"

if [ -f $MOV_DIR/$MOV_FILE_IN ]
then

    echo "MOV FILE AVAILABLE & UPLOADING TO BANCO" | /bin/mail -s "Banco MOV File" -r "movfileuploader@ny11" "nseall@tworoads.co.in";

else

    echo "MOV FILE NOT AVAILABLE" | /bin/mail -s "Banco MOV File" -r "movfileuploader@ny11" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in"

fi

cp $MOV_DIR/"MOVFUT_35786_"${YYYYMMDD:4:2}"_"${YYYYMMDD:6:2}"_"${YYYYMMDD:0:4}".txt" /tmp/$MOV_FILE

#### SFTP ####

sftp -b - -oPort=20681 -oIdentityFile=$RSA_KEY $USER@$BANCO_SFTP << !

cd out
put /tmp/$MOV_FILE
bye

!

#############

rm /tmp/$MOV_FILE
