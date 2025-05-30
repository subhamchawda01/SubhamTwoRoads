#!/bin/bash
if [ $# -lt 2 ]; then
    echo "USAGE: $0 query-id risk-tags"
    exit
fi

PROGID=$1
GIVEN_RISK_TAGS=$2

CURRENT_TIME=`date +"%s"` ;
FILE_TIME="$CURRENT_TIME";

#######Change time for ASX and chi to get correct log files#######
HOST_EXCH=`hostname | awk -F"-" '{print $2}'`;
if [ "$HOST_EXCH" == "chi" ]; then
    FILE_TIME=$((CURRENT_TIME + 7200));
elif [ "$HOST_EXCH" == "ASX" ]; then
	FILE_TIME=$((CURRENT_TIME + 14400));
fi
################################################
YYYYMMDD=`date -d @"$FILE_TIME" +%Y%m%d`

log_file=/spare/local/logs/tradelogs/log.$YYYYMMDD.$PROGID
query_end_time="";
query_start_time="";

#get query start and end time from cron
query_start_time=`crontab -l | grep -w $PROGID | grep start | awk '{print $2$1}' | head -1`; #another start msg for user_msg start, pick any
query_end_time=`crontab -l | grep -w $PROGID | grep stop | awk '{print $2$1}' | tail -1`;    

if [ "$query_start_time" == "" ]; then
	echo "ERROR: query start time not found in cron. using 0001";
	query_start_time="0001";
fi

if [ "$query_end_time" == "" ]; then
	echo "ERROR: query end time not found in cron. using 2359";
	query_end_time="0001";
fi

echo "Looking for risk params in $log_file";

#Notify risk client of the SACI-queryid-tag triplet
sleep 5  #safe wait

YYYYMMDD=`date +%Y%m%d` ;
found_pair=0
num_attempts=0
while [ "$found_pair" -eq 0 -a "$num_attempts" -lt 360 ];	#wait for half an hour awaiting all saci dumps
do
    num_attempts=$(($num_attempts+1))
    echo "Trying to find our SACI-Tag pair ... Attemp # $num_attempts"
    #find the latest SACI-tag pair(s) printed
    if [ `tac $log_file | awk '{if(flag!=1) print $0; if ($0 ~ /ClientLoggingSegmentInitializerE:Initialize/) {flag=1}}' | tac | grep DUMPEDALLSACIS | wc -l` -ge 0 ]; then
	
	for saci in `tac $log_file | awk '{if(flag!=1) print $0; if ($0 ~ /ClientLoggingSegmentInitializerE:Initialize/) {flag=1}}' | tac | grep "RMC_SACI" | awk '{print $2}'`;
	do
	    found_pair=1
	    echo "Notifying of triplet: $PROGID $saci $GIVEN_RISK_TAGS $query_start_time $query_end_time from file $log_file";
	    /home/pengine/prod/live_execs/risk_control $PROGID $saci $GIVEN_RISK_TAGS $query_start_time $query_end_time
	done
    fi
    if [ "$found_pair" -eq 0 ]; then
	#Retry after 5 seconds
	echo "Could not find our SACI-Tag pair ! Retrying ..."
	sleep 5
    fi
done
