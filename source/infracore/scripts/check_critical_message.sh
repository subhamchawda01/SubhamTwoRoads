#!/bin/bash

SLACK_EXEC="/home/pengine/prod/live_execs/send_slack_notification" ;

DATE=`date +%Y%m%d` ;

if [ $# -gt 0 ] ; then 
	DATE=$1 ;
fi

alert_str="NumCriticalAlerts: " ;
send="0"

cd /spare/local/logs/tradelogs/ ;


for i in log.$DATE.* ; do 

	num_alerts=`zgrep "CRITCAL:" $i | wc -l` ; 

	if [ $num_alerts -gt 0 ] ; then
		alert_str=$alert_str"|"$i"->"$num_alerts ;
		send="1" ;
	fi
done

#echo $alert_str


num_drops=`grep -i drop /spare/local/MDSlogs/combined_writer_dbg_$DATE.log | wc -l`
num_drops="NumPacketDrops::"$num_drops ;
#echo "NumPacketDrops : "$num_drops ;

if [ $send -gt 0 ] ; then
	$SLACK_EXEC nse-prod-issues DATA $alert_str ;
	$SLACK_EXEC nse-prod-issues DATA $num_drops ;
fi
