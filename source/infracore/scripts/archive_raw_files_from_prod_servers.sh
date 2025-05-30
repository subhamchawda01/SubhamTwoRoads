#!/bin/bash
#This script archives all the raw files on prod servers to NAS

time_now=`date +%s`
today_=`date +%Y%m%d`
file_list=files_$time_now
NAS_IP=10.23.74.40
remote_user=dvcinfra
SERVER=`hostname`
temp_archive=$SERVER.$time_now.raw_files.tar.gz
remote_location=/apps/old_logs/$SERVER/RawDataNew/
local_location=/spare/local/MDSlogs/RawDataNew/

#Copy to NAS and delete from prod server
cd $local_location;

#Prepare the list of files to be archived
find ./ -name "*$1_$today_.raw" > $file_list
bkp_files=`cat $file_list | tr '\n' ' '`

#Archive all raw files
tar -zcvf $temp_archive $bkp_files

#Previous step failed => do not proceed ahead
if [ $? -ne 0 ]; then
echo "Archiving files failed"
rm $temp_archive
rm $file_list
exit 2
fi

#Send the archived tar file to NAS
rsync -raz $temp_archive $remote_user@$NAS_IP:

#Previous step failed => do not proceed ahead
if [ $? -ne 0 ]; then
echo "Could not send archived file to NAS"
rm $temp_archive
rm $file_list
exit 2
fi

#ssh to NAS server and unpack the archive to appropriate location
ssh $remote_user@$NAS_IP << UNPACK_SCRIPT

#Create directory if doesn't exist, and move there
mkdir -p $remote_location
cd $remote_location

#Simply untar the archive (since we are already in the right directory)
tar -zxvf /home/$remote_user/$temp_archive

#Remove the tar file (only if it was extracted successfully)
if [ \$? -eq 0 ]; then
rm /home/$remote_user/$temp_archive
fi

UNPACK_SCRIPT

#Remove the archived files in case the transfer was successful
for file in `cat $file_list | tr '\n' ' '`;
do
echo "Removing file $file"
rm $file
done

#Clean up temp files
rm $file_list
rm $temp_archive
