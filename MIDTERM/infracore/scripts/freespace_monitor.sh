#!/bin/bash

USAGE1="$0 SERVER NY_THRESHOLD TRD_THRESHOLD"
EXAMP1="$0 ALL 95 80"

if [ $# -ne 3 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

#===================== SERVER - Currently only ALL required ======================#

SERVER=$1; shift ; 

#=================================================================================#

#============================== Threshold ========================================#

DEV_THRESHOLD=$1; shift ;
TRD_THRESHOLD=$1; shift ;

#=================================================================================#

SERVERS_SCRIPT="$HOME/infracore/GenPerlLib/print_all_machines_vec.pl"
dev_server=(`perl $SERVERS_SCRIPT D`);
trd_server=(`perl $SERVERS_SCRIPT T`);
PROD_OR_DEV="DEV"

MAIL_FILE=/tmp/space_monitor.tmp
>$MAIL_FILE

#============================================== DEV Servers ==================================# 

for server in "${dev_server[@]}" 
do

    current_utilization_raw=`ssh $server 'df -h | grep "/home"'`

    current_utilization_processed=`echo $current_utilization_raw | awk '{print $(NF-1)}' | awk -F"%" '{print $1}'` 

    data_space=`ssh $server 'du -sh /apps/data 2>/dev/null' | awk '{print $1}'`
    spare_local=`ssh $server 'du -sh /spare/local/logs 2>/dev/null' | awk '{print $1}'`

    if [ $current_utilization_processed -ge $DEV_THRESHOLD ] 
    then 

	echo "DEV SERVER : " $server " Space Snapshot : " $current_utilization_raw " AppsData : " $data_space " SpareLocalLogs : " $spare_local >> $MAIL_FILE ;

    fi

done

#==================================================================================#

#=========================================== TRD Servers =======================================#

for server in "${trd_server[@]}" 
do

    current_utilization_raw=`ssh $server 'df -h | grep "/home"'`

    current_utilization_processed=`echo $current_utilization_raw | awk '{print $(NF-1)}' | awk -F"%" '{print $1}'` 

    data_space=`ssh $server 'du -sh /spare/local/MDSlogs 2>/dev/null' | awk '{print $1}'`
    spare_local=`ssh $server 'du -sh /spare/local/logs 2>/dev/null' | awk '{print $1}'`

    max_disk_usage=`ssh $server 'df -P | grep -v "MLNX_OFED" | grep -v "Filesystem" | grep -v "/boot"' | awk '{print $5}' | cut -d'%' -f1 | awk '{if($1>max_val)max_val=$1} END{print max_val}'`
#if [ $current_utilization_processed -ge $TRD_THRESHOLD ]
    if [ $max_disk_usage -ge $TRD_THRESHOLD ]
    then

	echo "PROD SERVER : " $server " Space Snapshot : " $current_utilization_raw " MDSData : " $data_space " SpareLocalLogs : " $spare_local " Max_Disk_Usage: " $max_disk_usage >> $MAIL_FILE ;
        PROD_OR_DEV="PROD"

    fi

done

#==================================================================================#

LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then
#cat $MAIL_FILE
/bin/mail -s "DiskSpaceAlert - $PROD_OR_DEV" -r "nseall@tworoads.co.in" "nseall@tworoads.co.in" < $MAIL_FILE

fi

rm -rf $MAIL_FILE
