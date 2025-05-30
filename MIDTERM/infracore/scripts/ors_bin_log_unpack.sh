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
exch_timing=$4;
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

# Create the needed directory structure on the server
if [ ! -d ${YYYYMMDD:0:4} ] ; then mkdir ${YYYYMMDD:0:4} ; fi

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

cd ${YYYYMMDD:0:4} # YYYY/
if [ ! -d ${YYYYMMDD:4:2} ] ; then mkdir ${YYYYMMDD:4:2} ; fi
cd ${YYYYMMDD:4:2} # MM/
if [ ! -d ${YYYYMMDD:6:2} ] ; then mkdir ${YYYYMMDD:6:2} ; fi
cd ${YYYYMMDD:6:2} # DD/

dest_path=`echo ${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}`

# Showing the message after we've already moved into the directory.
echo -ne $0" : Moving to "
pwd

mv $ORSDATABASEDIR/$CURRENT_LOCATION/$ARCH_NAME.gz ./

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

# Unarchive                                                    
gunzip -f $ARCH_NAME.gz

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

for file in `tar -xvf $ARCH_NAME`
do
   FILENAME="data/ORSData/$CURRENT_LOCATION/$dest_path/$file";
   /apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd put /apps/$FILENAME s3://s3dvc/NAS1/$FILENAME;

   #Upload to HS1 server (EC2 file host) as well
   file="/NAS1/"$FILENAME ;
   hs1_disk=`$HOME/get_hs1_path $file`; #returns /media/ephemeral?
   path="$hs1_disk/s3_cache$file" ;
   enclosing_folder=`dirname $path` ;
   path_temp=$path ;
   path_temp+="_temp" ;
   s3_path="s3://s3dvc"$file ;
   ssh dvctrader@52.0.55.252 << FILE_UPDATE_SCRIPT
      mkdir -p $enclosing_folder ;
      /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd get --skip-existing --no-progress $s3_path $path_temp ;
      mv $path_temp $path ;
FILE_UPDATE_SCRIPT

done

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi

echo $0" : erasing "$ARCH_NAME
rm $ARCH_NAME

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi
