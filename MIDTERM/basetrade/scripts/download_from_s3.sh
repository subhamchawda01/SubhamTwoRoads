#!/bin/bash

# This script is run remotely by the mds_log_backup.sh script.
# -- 

USAGE="$0 MACHINE[NAS/HS1] EXCH LOC YYYYMMDD";

if [ $# -ne 4 ] ;
then
    echo $USAGE;
    exit;
fi

MACHINE=$1 #NAS/HS1
EXCHANGE=$2
CURRENT_LOCATION=$3
YYYYMMDD=$4
ARCH_NAME=$EXCHANGE$CURRENT_LOCATION$YYYYMMDD'.tar'

dest_dir=`echo $YYYYMMDD | sed 's/\(....\)\(..\)\(..\)/\1\/\2\/\3/' `
dest_path=`echo $YYYYMMDD | sed 's/\(....\)\(..\)\(..\)/\1\/\2\/\3/' `

if [ $EXCHANGE = "TMX" ] 
then

    EXCHANGE="TMX_FS" 

fi

if [ "$EXCHANGE" == "EUREX_NTA" ]
then

    EXCHANGE="EUREX"

fi

if [ "$EXCHANGE" == "EOBI" ]
then

    EXCHANGE="EOBINew"

fi

if [ "$EXCHANGE" == "QUINCY" ]
then

    EXCHANGE="QUINCY3"

fi

if [ "$EXCHANGE" == "OSEPriceFeed" ]
then

    EXCHANGE="OSENewPriceFeed"

fi

if [ "$EXCHANGE" == "OSEOrderFeed" ]
then

    EXCHANGE="OSE"

fi

if [ "$EXCHANGE" == "OSE_L1" ]
then

    EXCHANGE="OSENew_L1"

fi

path="data/"$EXCHANGE"LoggedData/"$CURRENT_LOCATION/$dest_path/;

if [ "$1" == "NAS" ]; then
	#Sync to NAS
	mkdir -m 777 -p /apps/$path
	/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd sync s3://s3dvc/NAS1/$path /apps/$path > /dev/null
else
	#Sync to HS1
	~/basetrade/scripts/send_from_s3_to_disks.sh s3://s3dvc/NAS1/$path $EXCHANGE $CURRENT_LOCATION $dest_path
fi

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi
