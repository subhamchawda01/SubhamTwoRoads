#!/bin/bash

rm -f /spare/local/files/expected_Q_1 /spare/local/files/running_Q_1 /spare/local/files/query_alert_mail 

for i in `ls /spare/local/logs/tradelogs/PID_TEXEC_DIR/` ; do x=`cat /spare/local/logs/tradelogs/PID_TEXEC_DIR/$i` ; echo "PID:"$x" " $i  ; done | sort > /spare/local/files/expected_Q_1 
ps -ef  | grep tradeinit | grep -v grep | grep LIVE | awk '{print "PID:"$2" QUERYID:"$NF " " $(NF-1) }' | sort > /spare/local/files/running_Q_1

join -1 1 -2 1 -a 1 -a 2 /spare/local/files/expected_Q_1 /spare/local/files/running_Q_1 \
| awk '{ if (NF==2) print "DEAD_QUERY: "  $0 ; if (NF==3) print "STRAY_QUERY: "  $0  ; }' > /spare/local/files/query_alert_mail

if [ -s /spare/local/files/query_alert_mail ] ; then 
  cat /spare/local/files/query_alert_mail | sed 's/_PIDfile.txt//g' | awk -F"/" '{print $1}'
else 
  echo "DEAD/STRAY QUERIES WERE TAKEN CARE OFF" ; 
fi 

rm /spare/local/files/query_alert_mail
