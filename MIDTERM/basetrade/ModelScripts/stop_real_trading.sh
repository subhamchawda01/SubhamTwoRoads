#!/bin/bash

USAGE="$0 EXCH PROGID";
if [ $# -ne 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRADE_EXEC=$HOME/LiveExec/bin/tradeinit

EXCH=$1; shift;
PROGID=$1; shift;

OUTPUTLOGDIR=/spare/local/logs/tradelogs ; 
PIDDIR=$OUTPUTLOGDIR/PID_TEXEC_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$PROGID"_"PIDfile.txt

cd $HOME

case $EXCH in
    cme|CME)
	;;
    eurex|EUREX)    
	;;
    tmx|TMX)    
	;;
    bmf|BMF)    
	;;
    *)    
	echo EXCH should be CME, EUREX, TMX or BMF
	;;
esac

if [ -f $PIDFILE ] ; then
    
    num_lines=`wc -l $PIDFILE | awk '{print $1}'` ;
    if [ $num_lines -gt 0 ] ; then
	
	TRADE_EXEC_PID=`tail -1 $PIDFILE`
	kill -2 $TRADE_EXEC_PID # SIGINT
	sleep 3;

	running_proc_string=`ps -p $TRADE_EXEC_PID -o comm=`;
	if [ $running_proc_string ] ; then 
#	    echo "patience ... "; 
	    sleep 4 ; 
	    
	    running_proc_string=`ps -p $TRADE_EXEC_PID -o comm=`;
	    if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		echo "Sending SIGKILL $PROGID "; 
		kill -9 $TRADE_EXEC_PID
	    fi
	fi
	
	rm -f $PIDFILE
	exit;
    else
	# print error & exit
#	echo "Cannot stop an instance of $TRADE_EXEC LIVE $PROGID since $PIDFILE has no lines"
	exit;
    fi    
else
    # print error & exit
#    echo "Cannot stop an instance of $TRADE_EXEC LIVE $PROGID since $PIDFILE does not exist"
    exit;
fi    
