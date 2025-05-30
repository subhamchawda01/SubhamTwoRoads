#!/bin/bash

# USAGE:
# 
# tars & gunzips the logs saved to the directories in '/spare/local/ORSBCAST/'
# Sends them over to the specified exchange.
# -- ramkris
# parent file: mds_log_backup.sh


USAGE="$0    EXCHANGE    CURRENT_LOCATION    YYYYMMDD";
USAGE_DESC="Syncs logs for exchange 'EXCHANGE' dated 'YYYYMMDD' to 'CURRENT_LOCATION' server.";

if [ $# -ne 3 ] ;
then
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

USER="dvcinfra"

EXCHANGE=$1;
CURRENT_LOCATION=$2;
YYYYMMDD=$3;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

DATA_CONVERTER_EXEC=/home/pengine/prod/live_execs/generic_data_converter ;

/home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 1 >/dev/null 2>&1

for files in `ls /spare/local/MDSlogs/GENERIC/ORS*$YYYYMMDD* | grep "ORS_NSE"`
do

   `$DATA_CONVERTER_EXEC ORS $YYYYMMDD $files /spare/local/MDSlogs /spare/local/ORSBCAST $EXCHANGE` ;

done

DEST_SERVER="10.23.5.67"

if [[ $HOSTNAME == *tok* || $HOSTNAME == *TOK* ]]
then

    DEST_SERVER="10.134.126.26"

fi

if [[ $HOSTNAME == *mos* || $HOSTNAME == *MOS* || $HOSTNAME == *hk* || $HOSTNAME == *HK* ]]
then
    DEST_SERVER="10.23.199.51";
fi


ORSBCAST_SRC="/spare/local/ORSBCAST";
ORSDATABASEDIR=/NAS1/data/ORSData

echo $0" : Moving into " $ORSBCAST_SRC"/"$EXCHANGE

cd $ORSBCAST_SRC
cd $EXCHANGE

gzip -f *$YYYYMMDD

# copy from ORSBCAST_SRC/EXCHANGE to ORSDATABASEDIR/CURRENT_LOC

# Archiving the logs.
ARCH_NAME=$EXCHANGE.tar
echo $0" : Archiving logs to "$ARCH_NAME.gz
tar -cf ../$ARCH_NAME *$YYYYMMDD*

# Store the return code for error checking.
echo $? > /tmp/$EXCHANGE"_"$YYYYMMDD

cd ..
gzip -f $ARCH_NAME

echo $0" : Sending logs to "$DEST_SERVER

# Must add RSA public key for this to work without prompting for a password.
scp $ARCH_NAME.gz $USER@$DEST_SERVER:$ORSDATABASEDIR/$CURRENT_LOCATION

# Store the return code for error checking.
echo $? >> /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : Connecting to "$DEST_SERVER
# Must add RSA public key for this to work without prompting for a password.
# Run local script on remote server.
ssh $USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/ors_bin_log_unpack.sh $EXCHANGE $CURRENT_LOCATION $YYYYMMDD

# Store the return code for error checking.
echo $? >> /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : erasing "$ARCH_NAME.gz
rm -f $ARCH_NAME.gz

# Find if there was an error
ERRORCODE=$(grep -v "0" /tmp/$EXCHANGE"_"$YYYYMMDD | wc -l)

if [ $ERRORCODE != "0" ] ;
then
    echo $0" : ERROR => Copy FAILED."
    exit;
fi

echo $0" : SUCCESS => Copy completed."

# Only delete this file if copy succeeded.
rm -f /tmp/$EXCHANGE"_"$YYYYMMDD
