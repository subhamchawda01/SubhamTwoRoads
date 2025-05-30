#!/bin/bash

USAGE1="$0 SERVER NY_THRESHOLD TRD_THRESHOLD"
EXAMP1="$0 ALL 90 80"

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

#==================================================================================#

MAIL_FILE=/tmp/inode_monitor.tmp
>$MAIL_FILE

#============================================== DEV Servers ==================================# 

for server in "${dev_server[@]}" 
do

    current_utilization_raw=`ssh $server 'df -i | grep "/home"'` #inode 

    current_utilization_processed=`echo $current_utilization_raw | awk '{print $(NF-1)}' | awk -F"%" '{print $1}'` 

    if [ $current_utilization_processed -ge $DEV_THRESHOLD ] 
    then 

	echo "SERVER : " $server " Inode Snapshot : " $current_utilization_raw >> $MAIL_FILE ;

    fi

done

#==================================================================================#

#=========================================== TRD Servers =======================================#

for server in "${trd_server[@]}" 
do

    current_utilization_raw=`ssh $server 'df -i | grep "/home"'`

    current_utilization_processed=`echo $current_utilization_raw | awk '{print $(NF-1)}' | awk -F"%" '{print $1}'` 

    if [ $current_utilization_processed -ge $TRD_THRESHOLD ]
    then

	echo "SERVER : " $server " Inode Snapshot : " $current_utilization_raw >> $MAIL_FILE ;

    fi

done

#==================================================================================#

LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then

    /bin/mail -s "InodeUsageAlert" -r "InodeCountMonitor@ny11" "nseall@tworoads.co.in" < $MAIL_FILE

fi

rm -rf $MAIL_FILE
