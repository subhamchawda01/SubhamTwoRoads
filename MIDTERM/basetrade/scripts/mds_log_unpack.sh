#!/bin/bash

# This script is run remotely by the mds_log_backup.sh script.
# -- 

USAGE="$0 EXCH LOC YYYYMMDD";

if [ $# -ne 3 ] ;
then
    echo $USAGE;
    exit;
fi

EXCHANGE=$1
CURRENT_LOCATION=$2
YYYYMMDD=$3
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


dest_dir=/apps/data/$EXCHANGE'LoggedData'/$CURRENT_LOCATION/$dest_dir

host=`hostname | awk -F"-" '{print $2}'` ;
upload_dir="/NAS1" ;

if [ "$host" == "HK" ] || [ "$host" == "TOK" ] || [ "$host" == "MOS" ] || [ "$host" == "hk" ] || [ "$host" == "tok" ] || [ "$host" == "mos" ]
then

  upload_dir="/apps" ;

fi


if [ ! -d $dest_dir ] ; then mkdir -m 777 -p $dest_dir
fi

AWS_TRIGGER_DIR=/apps/data/AWSTrigger 

if [ ! -d $AWS_TRIGGER_DIR ] ; then mkdir -m 777 -p $AWS_TRIGGER_DIR ; fi 
UNIQ_TIME=`date +"%s.%N"`
AWS_TRIGGER_FILE=$AWS_TRIGGER_DIR/mds_datacopy_$UNIQ_TIME 

>$AWS_TRIGGER_FILE

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

cd $dest_dir
if [ $? != 0 ] ; then exit $? ; fi
ls -l "/apps/data/"$ARCH_NAME 
mv "/apps/data/"$ARCH_NAME .
if [ $? != 0 ] ; then exit $? ; fi

#Sync to HS1
HS1_TMP_PATH="/tmp"
rsync $ARCH_NAME dvctrader@52.0.55.252:$HS1_TMP_PATH/
SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
ssh -n -f  $SSH_VARS dvctrader@52.0.55.252 "~/basetrade/scripts/send_to_disks.sh $HS1_TMP_PATH/$ARCH_NAME $EXCHANGE $CURRENT_LOCATION $dest_path &>/dev/null &"

if [ $? != 0 ] ; then exit $? ; fi

#Sync to S3 [Files or folders?]
tar -xvf $ARCH_NAME
if [ $? != 0 ] ; then exit $? ; fi

rm $ARCH_NAME

path="data/"$EXCHANGE"LoggedData/"$CURRENT_LOCATION/$dest_path/;
/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd sync /apps/$path s3://s3dvc/NAS1/$path > /dev/null

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi
