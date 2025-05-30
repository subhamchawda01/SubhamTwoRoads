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


USER='dvcapital' ;
BCS_SFTP='89.249.27.193' ;
PORT=2222
export SSHPASS=QhWmPc ;


# fetch
mkdir -p $HOME/bcs_files/$YYYYMMDD ;
cd $HOME/bcs_files/$YYYYMMDD ;

"sftp -oPort=$PORT -oBatchMode=no -b - $USER@$BCS_SFTP  << !"
sftp -oPort=$PORT -oBatchMode=no -b - $USER@$BCS_SFTP  << !

   get public/*/*$YYYYMMDD*
   bye
!

#transfer
OUT_DIR=/apps/broker_files/BCS_MOS/
rsync -avpz $HOME/bcs_files/$YYYYMMDD dvcinfra@10.23.74.40:$OUT_DIR/

#clean
cd $HOME/bcs_files/ ;
rm -rf $YYYYMMDD ;
