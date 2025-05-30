#!/bin/bash
#This script finds the worker with lowest load[workers listed in instances.txt]
#and schedules job on it. 
#Arguments:
# BACKGROUND: 0 -> donot run in background, 1 -> run in background

USAGE="$0 BACKGROUND[0/1] USER[$USER/dvctrader] EXEC ARGS ";
if [ $# -lt 3 ] ; 
	then 
	echo $USAGE;
	exit;
fi

LOGFILE=/spare/local/$USER/manual_run_logs/`date +%s%N`
BACKGROUND=$1; shift;
if [ "$BACKGROUND" != "0" ] && [ "$BACKGROUND" != "1" ]; then
	echo "Background variable should be 0 or 1";
	exit 1;
fi

USER=$1; shift
EXEC=`echo "$@"`
SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
minm=1000
final_loc=''
path=`dirname $0`

for loc in `cat $path/instances.txt`; do
 	output=`ssh $SSH_VARS $USER@$loc 'cat /proc/loadavg' 2> /dev/null`;
	if [ "$output" != "" ]; then
		load=`echo $output | awk -F' ' '{print $1}'`
		cond=`echo $load'<'$minm | bc -l`
		if [ "$cond" -eq 1 ]; then
			minm=$load
			final_loc=$loc
		fi
	fi
done

if [ "$final_loc" != "" ]; then
	if [ "$BACKGROUND" -eq 0 ]; then
		echo "Running at: $final_loc"
		ssh $SSH_VARS $USER@$final_loc "$EXEC";
	else
		echo "Running at: $final_loc Logfile: $LOGFILE"
		ssh -n -f $SSH_VARS $USER@$final_loc "sh -c 'mkdir -p `dirname $LOGFILE`; $EXEC &> $LOGFILE &'";
	fi
fi
