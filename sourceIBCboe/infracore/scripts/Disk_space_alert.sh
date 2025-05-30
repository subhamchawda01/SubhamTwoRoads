#!/bin/bash/

diskspace_file="/tmp/disk_space_report.txt"

disk_space=` df -h | awk '{print $5, $6}' | awk '{if($1>90) print $0}' | grep  .[0-9]`
disk_inode=` df -i | awk '{print $5, $6}' | awk '{if($1>90) print $0}' | grep  .[0-9]`

echo -e "space:\n\n $disk_space\n\n inode:\n\n $disk_inode" > $diskspace_file

cat $diskspace_file | mail -s "--WORKER ALERT--DISKSPACE/INODE---NEED ATTENTION" hardik.dhakate@tworoads-trading.co.in
