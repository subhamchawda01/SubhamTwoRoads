#!/bin/bash

#This script fetches all files from s3 bucket which are not already present locally
#Meant to be run only on HS1 server (EC2)
#s3cmd-1.5.0-rc1/s3cmd ls s3://s3dvc/NAS1/data/ | grep -v "JGB_\|TPX_" | awk '{print substr($NF,11)}' > folders_to_download

ctr=0
for folder in `cat folders_to_download`;
do
    #Create a new thread for each folder in s3 bucket (for parallel download)
    ./fetch_folder_new_thread.sh $folder $ctr >> /media/ephemeral22/log_$ctr &
    ctr=$((ctr+1))
done
