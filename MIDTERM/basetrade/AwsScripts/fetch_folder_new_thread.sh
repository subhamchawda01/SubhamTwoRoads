#!/bin/bash

#This script fetches a single folder from S3 bucket
#USAGE: <script> folder_name counter (file_counter) should contain the files in that folder
#To be run only on HS1 file server

folder=$1
ctr=$2
s3cmd-1.5.0-rc1/s3cmd ls --recursive s3://s3dvc$folder* | awk '{print substr($NF,11);}' > files_$ctr
for file in `cat files_$ctr`;
do
    #Get disk number using Hash function
    disk=`/home/dvctrader/get_path $file`
    path="$disk/s3_cache$file"
    enclosing_folder=`dirname $path`
    echo $disk ", " $path ", " $enclosing_folder ", " $file
    if [ ! -e $path ]
    then
        mkdir -p $enclosing_folder
        s3_path="s3://s3dvc$file"
        echo "s3 path: " $s3_path
        #Fetch the file from S3
        /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd get --no-progress $s3_path $path
    fi
done
