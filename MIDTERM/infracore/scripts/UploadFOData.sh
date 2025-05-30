#!/bin/bash

TODAY=`date +"%Y%m%d"` ;

[ -f /home/dvcinfra/$TODAY'_'FO.db ] || echo "REMOTE FILE COUNT UNKNOWN..." ; 
[ -f /home/dvcinfra/$TODAY'_'FO.db ] || exit ; 

local_file_count=`cat /home/dvcinfra/$TODAY'_'FO.db` ; 
remote_file_count=0 ; 
threshold_count=`echo $local_file_count | awk '{printf "%d", $1*0.3}'` ; 
current_file_count=0 ; 

echo "SYNC STATUS, LOCAL FILE COUNT -> $local_file_count, THRESHOLD TRIGGER -> $threshold_count, CURRENT FILE COUNT -> $current_file_count" >> fo_$TODAY"_sync_status.txt" ; 

while [ $current_file_count -lt $threshold_count ] ; do 
  sleep 30 ;  
  current_file_count=`find /home/dvcinfra/TBTData/${TODAY:0:4}/${TODAY:4:2}/${TODAY:6:2} -type f | wc -l` ; 
  echo "SYNC STATUS, LOCAL FILE COUNT -> $local_file_count, THRESHOLD TRIGGER -> $threshold_count, CURRENT FILE COUNT -> $current_file_count" >> fo_$TODAY"_sync_status.txt" ; 
done 

echo "THERE ARE ENOUGH FILES NOW TO START UPLOADING PROCESS...STARTING UPLOAD..." >> fo_$TODAY"_sync_status.txt" ;

while [ $remote_file_count -lt $local_file_count ] ; do

  rsync -avz --progress /home/dvcinfra/TBTData/${TODAY:0:4}/${TODAY:4:2}/${TODAY:6:2}/* dvcinfra@10.23.74.52:/spare/local/MDSlogs/NSE_FO >>fo_$TODAY"_sync_status.txt" ; 
  remote_file_count=`ssh 10.23.74.52 "find /spare/local/MDSlogs/NSE_FO -type f | grep '$TODAY' | wc -l"` ; 

  echo "RSYNC ROUND COMPLETED AT : " `date` " WITH REMOTE_FILE_COUNT -> $remote_file_count, AGAINST EXPECTED LOCAL COUNT -> $local_file_count" >> fo_$TODAY"_sync_status.txt" ;

done

rm -rf fo_$TODAY"_sync_status.txt" ; 
rm -rf /home/dvcinfra/$TODAY'_'FO.db ; 
rm -rf /home/dvcinfra/TBTData/* ; 

#Initiate NY12 Sync  
ssh dvcinfra@10.23.74.52 "mkdir -p /spare/local/MDSlogs/NSE; mv /spare/local/MDSlogs/NSE_FO/* /spare/local/MDSlogs/NSE/" ; 


local_cd_file_count=`cat /home/dvcinfra/$TODAY'_'CD.db` ;
local_cm_file_count=`cat /home/dvcinfra/$TODAY'_'CM.db` ;

total_file_count=`echo $local_file_count" "$local_cd_file_count" "$local_cm_file_count | awk '{print $1+$2+$3}'` ;
remote_file_count=0 ; 

while [ $remote_file_count -lt $total_file_count ] ; do 

  sleep 10 ; 
  remote_file_count=`ssh 10.23.74.52 "find /spare/local/MDSlogs/NSE -type f | grep '$TODAY' | wc -l"` ;

done

ssh -n -f 10.23.74.52 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh NSE NSE $TODAY >/dev/null 2>&1 &" &
