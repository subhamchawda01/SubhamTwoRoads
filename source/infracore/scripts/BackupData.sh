#!/bin/bash

#Let's wait until all raw dump processes are completed 
while [ `ps -ef | grep dump_raw_multicast_data_using_config  | grep -v grep | wc -l` -gt 0 ] ; do 
  sleep 60 ;
done 

#Let's wait until datacopy is done as well 
while [ `ps -ef | grep ConvertAndUploadData | grep -v grep | wc -l` -gt 0 ] ; do
  sleep 60 ; 
done 

#Initiate Compression and Move
gzip /spare/local/RawData/dump_raw_multicast_data_using_config_*  ; 
mv /spare/local/RawData/dump_raw_multicast_data_using_config_* /spare/local/RawData/OldData/ ; 

#Initiate Backup Now 

start_time=`date +"%s"` ; 

for files in `ls /spare/local/RawData/OldData/*` ; do

  local_size=`ls -lrt $files | awk '{print $5}'` ;
  remote_size=0 ; 

  while [ $remote_size -lt $local_size ] ; do 

    rsync -avz --progress $files dvcinfra@10.23.5.161:/home/dvcinfra/RawData/ >>rsync_backup_data.log 2>>rsync_backup_data.log ;
    file=`echo $files | awk -F"/" '{print $NF}'` ; 
    remote_size=`ssh dvcinfra@10.23.5.161 "ls -lrt /home/dvcinfra/RawData/$file" | awk '{print $5}'` ;

    current_time=`date +"%s"` ; 
 
    #It's already 12 hours, something is wrong 
    if [ $((current_time-start_time)) -gt 43200 ] ; then 
      echo "SOMETHING WENT WRONG IN TRYING TO BACKUP NSE RAW DATA, THE SCRIPT SEEMS TO HAVE STUCK FOR 15 HOURS" | /bin/mail -s "NSE_RAWDATA_BACKUP_FAILED" ravi.parikh@tworoads.co.in nseall@tworoads.co.in ; 
      exit ; 
    fi 

  done 

done 

#Done With The Backup, Remove Secondary Files 
rm -rf /spare/local/RawData/OldData/*_secondary_* ; 
