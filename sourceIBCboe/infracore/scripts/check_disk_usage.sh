#!/bin/bash
  
diskspace_file="/tmp/disk_space_report.txt"

disk_space=`df -h | awk '{print $5, $6}' | awk '{if($1>90) print $0}' | grep  .[0-9]`
#disk_inode=`df -i | awk '{print $5, $6}' | awk '{if($1>80) print $0}' | grep  .[0-9]`

echo -e "SPACE:\n\n $disk_space" > $diskspace_file #\n\n inode:\n\n $disk_inode" > $diskspace_file
if [ `cat /tmp/disk_space_report.txt | wc -l` -gt 3 ]; then
  cat $diskspace_file | mail -s "--DISKSPACE ALERT 5.67---NEED ATTENTION" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in
fi

