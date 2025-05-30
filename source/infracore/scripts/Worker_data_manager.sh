#!/bin/bash

YYYYMMDD=`date +%Y%m%d`;
TODAY=$YYYYMMDD
file_to_ignore="/home/dvctrader/stable_exec/date_to_ignore_for_config_run"

touch $file_to_ignore

dir_to_check="/NAS1/data/NSELoggedData/NSE"
echo "Clearing NAS1"
mkdir -p $dir_to_check
n_path="NAS1"
disk_mount=`df -h | grep $n_path | wc -l`
if [[ $disk_mount -eq 1 ]]; then
	disk_rem=`df -h | grep $n_path | awk '{print $5}' | cut -d'%' -f1`
	day_check=80
	while [[ $disk_rem -gt 95  && $day_check -gt 60 ]];do	
                echo "$n_path Day: $day_check DISK REM: $disk_rem"
		find $dir_to_check -name "NSE_*" -type f -mtime +$day_check | grep -vf $file_to_ignore | grep NSE_ | grep NSELoggedData
		find $dir_to_check -name "NSE_*" -type f -mtime +$day_check | grep -vf $file_to_ignore | grep NSE_ | grep NSELoggedData | xargs rm;
		day_check=$(( day_check - 1 ))
		disk_rem=`df -h | grep $n_path | awk '{print $5}' | cut -d'%' -f1`
	done
else 
	echo "Disk Not Mounted $n_path"
fi
echo "$n_path Cleared"
dir_to_check="/spare/local/logs/tradelogs/"
mkdir -p $dir_to_check
n_path="NAS2"
echo "Clearing $n_path"
disk_mount=`df -h | grep $n_path | wc -l`
if [[ $disk_mount -eq 1 ]]; then
	disk_rem=`df -h | grep $n_path | awk '{print $5}' | cut -d'%' -f1`
        day_check=20
	while [[ $disk_rem -gt 98  && $day_check -gt 7 ]];do
                echo "$n_path Day: $day_check DISK REM: $disk_rem"
		find $dir_to_check -name "log.*" -type f -mtime +$day_check
		find $dir_to_check -name "log.*" -type f -mtime +$day_check -delete
                day_check=$(( day_check - 1 ))
                disk_rem=`df -h | grep $n_path | awk '{print $5}' | cut -d'%' -f1`
        done

else
        echo "Disk Not Mounted $n_path"
fi
echo "$n_path Cleared"

dir_to_check="/NAS4/data/ORSData"
mkdir -p $dir_to_check
n_path="NAS4"
echo "Clearing $n_path"
disk_mount=`df -h | grep $n_path | wc -l`
if [[ $disk_mount -eq 1 ]]; then
        disk_rem=`df -h | grep $n_path | awk '{print $5}' | cut -d'%' -f1`
        day_check=10
        while [[ $disk_rem -gt 98  && $day_check -gt 4 ]];do
                echo "$n_path Day: $day_check DISK REM: $disk_rem"
                find $dir_to_check -name "NSE*" -type f -mtime +$day_check
#                find $dir_to_check -name "NSE.*" -type f -mtime +$day_check -delete
                day_check=$(( day_check - 1 ))
                disk_rem=`df -h | grep $n_path | awk '{print $5}' | cut -d'%' -f1`
        done

else
        echo "Disk Not Mounted $n_path"
fi
echo "$n_path Cleared"

