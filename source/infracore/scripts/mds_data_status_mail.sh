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
  TODAY_DATE=`date +%Y%m%d`;
  YYYYMMDD=`$HOME/infracore_install/bin/calc_prev_day $TODAY_DATE`
fi 

if [ "$YYYYMMDD" == "TODAY" ]
then
  YYYYMMDD=`date +"%Y%m%d"`;
fi

FILE=$HOME/mds_data_stutus.txt
> $FILE

for loc in HK TOK CHI BSL FR2 BRZ CFE TOR MOS NY4 CRT;
do
  echo $loc >> $FILE;
  $HOME/infracore/scripts/check_data_NAS.sh $loc $YYYYMMDD >> $FILE
  echo "" >> $FILE
done;

$HOME/LiveExec/scripts/mail_file_to.sh Market_data_copy_status_$YYYYMMDD ravi.parikh@tworoads.co.in,vedant@tworoads.co.in,chandan.kumar@tworoads.co.in,nseall@tworoads.co.in $FILE vedant@tworoads.co.in
