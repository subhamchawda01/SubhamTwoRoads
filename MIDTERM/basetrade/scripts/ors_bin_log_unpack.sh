#!/bin/bash

# This script is run remotely by the ors_binary_log_backup.sh script.
# taken from 

USAGE="$0 EXCH LOC YYYYMMDD [exch_timing]";

if [ $# -lt 3 ] ;
then
    echo $USAGE;
    exit;
fi

EXCHANGE=$1
CURRENT_LOCATION=$2
YYYYMMDD=$3
exch_timing=$4
ARCH_NAME=$EXCHANGE.tar


ORSDATABASEDIR=/apps/data/ORSData

AWS_TRIGGER_DIR=/apps/data/AWSTrigger

if [ ! -d $AWS_TRIGGER_DIR ] ; then mkdir -m 777 -p $AWS_TRIGGER_DIR ; fi
UNIQ_TIME=`date +"%s.%N"`
AWS_TRIGGER_FILE=$AWS_TRIGGER_DIR/ors_bin_datacopy_$UNIQ_TIME

>$AWS_TRIGGER_FILE


if [ ! -d $ORSDATABASEDIR/$CURRENT_LOCATION ] ; 
then 
    mkdir $ORSDATABASEDIR/$CURRENT_LOCATION 
fi

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

cd $ORSDATABASEDIR/$CURRENT_LOCATION

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

dest_path=`echo ${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}`
if [ ! -d $dest_dir ] ; then mkdir -m 777 -p $dest_dir
fi

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

# Showing the message after we've already moved into the directory.
echo -ne $0" : Moving to "
pwd

mv $ORSDATABASEDIR/$CURRENT_LOCATION/$ARCH_NAME.gz ./

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

# Unarchive                                                    
gunzip $ARCH_NAME.gz

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

#Sync to HS1
HS1_TMP_PATH="/media/ephemeral16/temp_data_copy"
rsync $ARCH_NAME dvctrader@52.0.55.252:$HS1_TMP_PATH/
SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
ssh -n -f  $SSH_VARS dvctrader@52.0.55.252 "~/basetrade/scripts/send_to_disks.sh $HS1_TMP_PATH/$ARCH_NAME ORSData $CURRENT_LOCATION $dest_path $exch_timing &>/dev/null &"

if [ $? != 0 ] ; then exit $? ; fi

#Sync to S3 [Files or folders?]
tar -xvf $ARCH_NAME
if [ $? != 0 ] ; then exit $? ; fi

rm $ARCH_NAME

path="data/ORSData/$CURRENT_LOCATION/$dest_path/";
/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd sync /apps/$path s3://s3dvc/NAS1/$path > /dev/null

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi
