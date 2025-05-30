#!/bin/bash
SUB="Dead_Query_on_"`hostname`

while [ true ]
do
	for i in `ls /spare/local/logs/tradelogs/PID_TEXEC_DIR/` ; do x=`cat /spare/local/logs/tradelogs/PID_TEXEC_DIR/$i` ; echo $x" " $i  ; done | sort > /spare/local/files/expected_Q_1 
	ps -ef  | grep tradeinit | grep LIVE | awk '{print $2 " " $NF " " $(NF-1) }' | sort > /spare/local/files/running_Q_1

	sleep 15

	for i in `ls /spare/local/logs/tradelogs/PID_TEXEC_DIR/` ; do x=`cat /spare/local/logs/tradelogs/PID_TEXEC_DIR/$i` ; echo $x" " $i  ; done | sort > /spare/local/files/expected_Q_2 
	ps -ef  | grep tradeinit | grep LIVE | awk '{print $2 " " $NF " " $(NF-1) }' | sort > /spare/local/files/running_Q_2

	d1=`diff /spare/local/files/expected_Q_1 /spare/local/files/expected_Q_2`
        d2=`diff /spare/local/files/running_Q_1 /spare/local/files/running_Q_2`
	#checking diffs to avoid transient alerts
	if [ "$d1" == "" ] && [ "$d2" == "" ]
	then
		join -1 1 -2 1 -a 1 -a 2 /spare/local/files/expected_Q_1 /spare/local/files/running_Q_1 \
	 	| awk '{ if (NF==2) print "NOT_RUNNING: "  $0 ; if (NF==3) print "STRAY_QUERY: "  $0  ; }' > /spare/local/files/query_alert_mail
	fi

	if [ -s /spare/local/files/query_alert_mail ]  
	then /bin/mail -s $SUB -r "nseall@tworoads.co.in" "nseall@tworoads.co.in" < /spare/local/files/query_alert_mail   
	fi
	rm /spare/local/files/query_alert_mail
	
	sleep 45
done
