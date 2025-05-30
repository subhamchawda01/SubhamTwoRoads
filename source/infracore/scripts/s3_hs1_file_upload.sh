#!/bin/bash
USAGE="$0  FILENAME";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

FILENAME=$1

#Uploads to S3 in same path as given in $FILENAME
/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd put $FILENAME s3://s3dvc$FILENAME;

#Upload to EC2 file host as well
enclosing_folder=`dirname $FILENAME` ;
path_temp=$FILENAME ;
path_temp+="_temp" ;
s3_path="s3://s3dvc"$FILENAME 

#Uploads file to $path_temp first. In case this process fails and file copy is not completed, it can be started again
#Checking whether file copy is successful is done by presence of $FILENAME in EC2 file host
ssh dvctrader@52.0.55.252 << FILE_UPDATE_SCRIPT
  mkdir -p $enclosing_folder ;
  /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd get --skip-existing --no-progress $s3_path $path_temp ;
  mv $path_temp $FILENAME ;
FILE_UPDATE_SCRIPT

# If an error occurred, return the error, so CLEARDATA doesn't delete the files.
if [ $? != 0 ] ; then exit $? ; fi
