#!/bin/bash

TODAY=`date +"%Y%m%d"` ;

[ -f /home/dvcinfra/$TODAY'_'CD.db ] || echo "REMOTE FILE COUNT UNKNOWN..." ; 
[ -f /home/dvcinfra/$TODAY'_'CD.db ] || exit ; 

local_file_count=`cat /home/dvcinfra/$TODAY'_'CD.db` ; 
threshold_count=`echo $local_file_count | awk '{printf "%d", $1*0.3}'` ;
remote_file_count=0 ; 
current_file_count=0 ;

echo "SYNC STATUS, LOCAL FILE COUNT -> $local_file_count, THRESHOLD TRIGGER -> $threshold_count, CURRENT FILE COUNT -> $current_file_count" >> cd_$TODAY"_sync_status.txt" ;

while [ $current_file_count -lt $threshold_count ] ; do
  sleep 30 ;
  current_file_count=`find /home/dvcinfra/CDTBTData/${TODAY:0:4}/${TODAY:4:2}/${TODAY:6:2} -type f | wc -l` ;
  echo "SYNC STATUS, LOCAL FILE COUNT -> $local_file_count, THRESHOLD TRIGGER -> $threshold_count, CURRENT FILE COUNT -> $current_file_count" >> cd_$TODAY"_sync_status.txt" ;
done 

echo "THERE ARE ENOUGH FILES NOW TO START UPLOADING PROCESS...STARTING UPLOAD..." >> cd_$TODAY"_sync_status.txt" ;

while [ $remote_file_count -lt $local_file_count ] ; do

  rsync -avz --progress /home/dvcinfra/CDTBTData/${TODAY:0:4}/${TODAY:4:2}/${TODAY:6:2}/* dvcinfra@10.23.74.52:/spare/local/MDSlogs/NSE_CD >>cd_$TODAY"_sync_status.txt" ; 
  remote_file_count=`ssh 10.23.74.52 "find /spare/local/MDSlogs/NSE_CD -type f | grep '$TODAY' | wc -l"` ; 

  echo "RSYNC ROUND COMPLETED AT : " `date` " WITH REMOTE_FILE_COUNT -> $remote_file_count, AGAINST EXPECTED LOCAL COUNT -> $local_file_count" >>cd_$TODAY"_sync_status.txt" ; 

done

rm -rf cd_$TODAY"_sync_status.txt" ; 
rm -rf /home/dvcinfra/$TODAY'_'CD.db ; 

#Initiate NY12 Sync  
ssh dvcinfra@10.23.74.52 "mkdir -p /spare/local/MDSlogs/NSE; mv /spare/local/MDSlogs/NSE_CD/* /spare/local/MDSlogs/NSE/" ; 

rm -rf /home/dvcinfra/CDTBTData/*
