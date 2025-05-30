#!/bin/bash

USAGE1="$0 EXCH LOC YYYYMMDD CMD[START/STOP] <- LOC & YYYYMMDD unused.";
USAGE2="$0 EXCH LOC YYYYMMDD CMD[COPYDATA/CLEARDATA]";

if [ $# -ne 4 ] ; 
then 
    echo $USAGE1;
    echo $USAGE2;
    exit;
fi

ORSBCAST_LOG_EXEC=$HOME/LiveExec/bin/ors_binary_logger

COPY_LOG_EXEC=$HOME/LiveExec/scripts/ors_binary_log_backup.sh
CLEAR_LOG_EXEC=$HOME/LiveExec/scripts/ors_binary_clear_log.sh

EXCH=$1;
LOC=$2;
YYYYMMDD=$3;
CMD=$4;

PIDDIR=/spare/local/ORSBCAST/PID_ORSBCAST_DIR ;
mkdir -p $PIDDIR


PIDFILE=$PIDDIR/ORSBCASTLOGGER_$EXCH"_PIDfile.txt"


if [ $PWD != $HOME ] ; then cd $HOME ; fi

ORSBCAST_IP=127.0.0.1
ORSBCAST_PORT=12345

case $EXCH in
    CME)
	ORSBCAST_IP=225.2.3.2
	ORSBCAST_PORT=17107 
	;;
    
    EUREX)
	ORSBCAST_IP=225.2.3.1
	ORSBCAST_PORT=17117
	;;

    TMX)
	ORSBCAST_IP=225.2.3.3
	ORSBCAST_PORT=17127
	;;
    
    BMF)
	ORSBCAST_IP=127.0.0.1 #TODO to be changed to mcast ip
	ORSBCAST_PORT=17137 
	;;
    
    *)
	echo "Not implemented for $EXCH";
esac

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $ORSBCAST_LOG_EXEC --exch $EXCH --ip $ORSBCAST_IP --port $ORSBCAST_PORT since $PIDFILE exists"
	else
            # Appending the ors bcast logger pid to the same file -- 
	    # create the file
	    ls > $PIDFILE
	    $ORSBCAST_LOG_EXEC --exch $EXCH --ip $ORSBCAST_IP --port $ORSBCAST_PORT &
	    ORSBCASTLOGGERPID=$!
	    echo $ORSBCASTLOGGERPID > $PIDFILE
	fi    
	;;

    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
	    ORSBCASTLOGGERPID=`tail -1 $PIDFILE`
	    kill -2 $ORSBCASTLOGGERPID
	    sleep 1;

	    running_proc_string=`ps -p $ORSBCASTLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then 
		echo "patience ... "; 
		sleep 4 ; 

		running_proc_string=`ps -p $ORSBCASTLOGGERPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    echo "sending SIGKILL" ;
		    kill -9 $ORSBCASTLOGGERPID
		fi
	    fi

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $ORSBCAST_LOG_EXEC --exch $EXCH --ip $ORSBCAST_IP --port $ORSBCAST_PORT since $PIDFILE does not exist"
	fi    
	;;


    copylogs|COPYLOGS)

	;;
    clearlogs|CLEARLOGS)

	;;

    copydata|COPYDATA)
	$COPY_LOG_EXEC $EXCH $LOC $YYYYMMDD &
	;;

    cleardata|CLEARDATA)
	$CLEAR_LOG_EXEC $EXCH $YYYYMMDD &
	;;

    *)
	echo CMD $CMD not expected
	;;
esac
