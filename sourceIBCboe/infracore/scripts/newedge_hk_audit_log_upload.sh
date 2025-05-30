#!/bin/bash

USAGE1="$0 DATE"
EXAMP1="$0 YESTERDAY"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;

if [ "$YYYYMMDD" == "YESTERDAY" ]
then

    YYYYMMDD=`cat /tmp/YESTERDAY_DATE`

fi

if [ "$YYYYMMDD" == "TODAY" ]
then

    YYYYMMDD=`date +"%Y%m%d"` ;

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


DESTINATION_SUDIT_LOG_DIR=/apps/data/MFGlobalTrades/AUDIT

yyyy=`echo ${YYYYMMDD:0:4}` ;
mm=`echo ${YYYYMMDD:4:2}` ;
dd=`echo ${YYYYMMDD:6:2}` ;


this_dest_dir=$DESTINATION_SUDIT_LOG_DIR"/"$yyyy"/"$mm"/"$dd ;

cd $this_dest_dir ;

for files in `ls *HKFE*`
do

  zip $files".zip" $files ;
  rm -rf $files ;

done


HOST='ftp.newedgegroup.com';
USER='SDMA-DVCapital';
PASSWD='G71lrKqh#';

RSA_PRIVATE_KEY=/home/dvcinfra/.ssh/id_rsa.legacy

#### SFTP #### fetch cash statements txt

sftp -b - -oIdentityFile=$RSA_PRIVATE_KEY $USER@$HOST << !

cd incoming
put $this_dest_dir/*AuditTrail_HKFE*$YYYYMMDD*
bye

!

#############

