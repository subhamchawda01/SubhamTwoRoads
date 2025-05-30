#!/bin/bash

USAGE="$0 DATE HH:MM EXCHANGE_LIST_FILE TIME_INTERVAL";
if [ $# -lt 1 ]
then
    echo $USAGE;
    exit;
fi

DATE=$1; shift;
TIME_OFFSET=$1; shift;
EXCHANGE_LIST_FILE=$1; shift;
TIME_INTERVAL=$1; shift;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


CMD="date -d '$DATE $TIME_OFFSET:00' +%s";
#START_TIME=`date -d \'$DATE $TIME_OFFSET:00\' +%s`;
START_TIME=`eval $CMD`;
#echo $START_TIME;

echo ">>>> ~/infracore/scripts/create_bw_log.pl $EXCHANGE_LIST_FILE | sort -k1n > ~/bandwidth_tempfile.txt";
~/infracore/scripts/create_bw_log.pl $EXCHANGE_LIST_FILE | sort -k1n > ~/bandwidth_tempfile.txt;

echo ">>>> ~/infracore_install/bin/get_mds_bandwidth_from_time $TIME_INTERVAL ~/bandwidth_tempfile.txt $START_TIME";
~/infracore_install/bin/get_mds_bandwidth_from_time $TIME_INTERVAL ~/bandwidth_tempfile.txt $START_TIME;

rm -f ~/bandwidth_tempfile.txt;
