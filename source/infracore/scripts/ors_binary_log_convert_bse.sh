#!/bin/bash

USAGE="$0    EXCHANGE  YYYYMMDD";

if [ $# -ne 2 ] ;
then
    echo $USAGE
    exit;
fi
EXCHANGE=$1;
YYYYMMDD=$2;
[ $YYYYMMDD = "TODAY" ] && YYYYMMDD=$(date "+%Y%m%d");
DATA_CONVERTER_EXEC=/home/pengine/prod/live_execs/generic_data_converter ;
/home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 1 >/dev/null 2>&1

for files in `ls /spare/local/MDSlogs/GENERIC/ORS*$YYYYMMDD* | grep "ORS_BSE"`
do
   `$DATA_CONVERTER_EXEC ORS $YYYYMMDD $files /spare/local/MDSlogs /spare/local/ORSBCAST $EXCHANGE` ;
done

cd "/spare/local/ORSBCAST/$EXCHANGE"
taskset -c 3,4,5 gzip -f *$YYYYMMDD

mkdir -p /spare/local/MDSlogs/ORS_OLD_$YYYYMMDD
#mv /spare/local/MDSlogs/GENERIC/ORS*$YYYYMMDD* /spare/local/MDSlogs/ORS_OLD_$YYYYMMDD/

#find /spare/local/ORSBCAST/BSE -type f -mtime +6 -exec rm -f {} \;
#find /spare/local/ORSBCAST_MULTISHM/BSE -type f -mtime +6 -exec rm -f {} \;
