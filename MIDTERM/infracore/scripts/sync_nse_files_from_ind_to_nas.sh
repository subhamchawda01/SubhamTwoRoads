#!/bin/bash
USAGE="$0 YYYYMMDD NUM_FILES SRC[sdv-ind-srv11/sdv-ind-srv12] DATA[ALL/PARTIAL]";
if [ $# -ne 4 ] ;
then
    echo $USAGE
    exit;
fi
YYYYMMDD=$1
num_files_to_sync=$2
src=$3
data_to_sync=$4

logfile=/spare/local/logs/alllogs/nse_data_copy_logs
>$logfile
num_files_synced_from_nse=`ls -lrt /spare/local/MDSlogs/NSE_IND_BKP/*$YYYYMMDD.gz | wc -l`;
num_attempts=10;	#make 10 attempts to sync files from ind

#decide which line to use based on hostname
this_host_ip=$(hostname | awk -F"." '{print $1}');
if [ "$this_host_ip" == "localhost" ];then #ravi's machine
	line_to_use="BLR";
elif [ "$this_host_ip" == "sdv-sgx-srv11" ];then
	line_to_use="SGX";
else
	echo "This script should be called on either sdv-sgx-srv11 or blr_ravi_machine";
	exit;
fi

#decide which ip to use for ind11 and ind12 access from sgx/blr machine
ind_ip_to_use="";

if [ "$src" == "sdv-ind-srv11" ]; then
	if [ "$line_to_use" == "SGX" ]; then
		ind_ip_to_use="10.23.227.3";	
	elif [ "$line_to_use" == "BLR" ]; then
		ind_ip_to_use="10.23.115.61";
	fi 
	
elif [ "$src" == "sdv-ind-srv12" ]; then
	if [ "$line_to_use" == "SGX" ]; then
		ind_ip_to_use="10.23.227.2";	
	elif [ "$line_to_use" == "BLR" ]; then
		ind_ip_to_use="10.23.115.62";
	fi 
else
	echo "Incorrect Source machine. It should be either of IND11 or IND12";
	exit;
fi

while [ "$num_files_synced_from_nse" -lt "$num_files_to_sync" ] && [ "$num_attempts" -gt 0 ] ;do
	echo "start Sync from ind11 at `date`" > $logfile
	if [ "$data_to_sync" == "ALL" ]; then
		rsync -Lavz --progress  $ind_ip_to_use:/spare/local/MDSlogs/NSE_SGX/*$YYYYMMDD.gz /spare/local/MDSlogs/NSE_IND_BKP/ >/dev/null 2>&1
		rsync -Lavz --progress  $ind_ip_to_use:/spare/local/MDSlogs/NSE_BLR/*$YYYYMMDD.gz /spare/local/MDSlogs/NSE_IND_BKP/ >/dev/null 2>&1
	else
		rsync -Lavz --progress  $ind_ip_to_use:/spare/local/MDSlogs/NSE_$line_to_use/*$YYYYMMDD.gz /spare/local/MDSlogs/NSE_IND_BKP/ >/dev/null 2>&1
	fi
	num_files_synced_from_nse=`ls -lrt /spare/local/MDSlogs/NSE_IND_BKP/*$YYYYMMDD.gz | wc -l`;
	((num_attempts=num_attempts-1));
	sleep 10;
done

if [ "$num_files_synced_from_nse" -ge "$num_files_to_sync" ]; then
	echo "PULLFROMIND11SUCCESSFUL: completed Sync from ind11 at `date`" >> $logfile
else
	echo "PULLFROMIND11FAILED. Synced $num_files_synced_from_nse out of $num_files_to_sync " >> $logfile
	exit;
fi

#FIXME: can't say for sure if this is complete, making 10 rsync attempts before calling this a success
#num_files_synced_to_ny12=`ssh -q 10.23.74.52 "ls -lrt /spare/local/MDSlogs/NSE/*$YYYYMMDD.gz | wc -l"`;
num_attempts=10;	#make 10 attempts to sync files to ny12
while [ "$num_attempts" -gt 0 ] ;do
	if [ "$this_host_ip" == "sdv-sgx-srv11" ];then	#for sgx to ny12 limit to 4000 kbps(500KBps)
		rsync -avz --progress --bwlimit=500 /spare/local/MDSlogs/NSE_IND_BKP/*$YYYYMMDD.gz 10.23.74.52:/spare/local/MDSlogs/NSE/  >/dev/null 2>&1
	else
		rsync -avz --progress /spare/local/MDSlogs/NSE_IND_BKP/*$YYYYMMDD.gz 10.23.74.52:/spare/local/MDSlogs/NSE/  >/dev/null 2>&1
	fi
	#num_files_synced_to_ny12=`ssh -q 10.23.74.52 "ls -lrt /spare/local/MDSlogs/NSE/*$YYYYMMDD.gz | wc -l"`;
	((num_attempts=num_attempts-1));
	sleep 10;
done

#if [ "$num_files_synced_to_ny12" -ge "$num_files_to_sync" ]; then
	echo "PUSHTONY12SUCCESSFUL: completed Sync to ny12 at `date`" >> $logfile
#else
#	echo "PUSHTONY12FAILED. Synced $num_files_synced_to_ny12 out of $num_files_to_sync " >> $logfile
#	exit;
#fi
