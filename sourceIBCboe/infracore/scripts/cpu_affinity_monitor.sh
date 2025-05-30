#!/bin/bash

USAGE1="$0 CORE CMD[START/STOP]"

if [ $# -ne 2 ] ;
then 
    echo $USAGE1;
    exit;
fi

AFFINITY_MONITOR_EXEC=$HOME/LiveExec/bin/cpu_affinity_monitor 
AFFINITY_MONITOR_OUT_FILE=$HOME/affinity_monitor_cout_cerr.log

CORE=$1;
CMD=$2;

PIDDIR=/spare/local/logs/AffinityMonitor/PID_MDS_DIR ;

if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi

PIDFILE=$PIDDIR/AFFINITY_MONITOR_PIDfile.txt


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
        # print error & exit
            echo "Cannot start an instance of $AFFINITY_MONITOR_EXEC" ;
	else
        # Appending the mds logger pid to the same file -- 
	    
	    $AFFINITY_MONITOR_EXEC $CORE >$AFFINITY_MONITOR_OUT_FILE 2>$AFFINITY_MONITOR_OUT_FILE &

            AFFINITYMONITORPID=$!
            echo $AFFINITYMONITORPID > $PIDFILE

	fi
	;;

    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
            # Stop the mds_logger -- 
	    AFFINITYMONITORPID=`tail -1 $PIDFILE`
	    kill -2 $AFFINITYMONITORPID
	    sleep 1;

	    running_proc_string=`ps -p $AFFINITYMONITORPID -o comm=`;
	    if [ $running_proc_string ] ; then 
		echo "patience ... "; 
		sleep 4 ; 

		running_proc_string=`ps -p $AFFINITYMONITORPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    echo "sending SIGKILL" ;
		    kill -9 $AFFINITYMONITORPID
		fi
	    fi

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $AFFINITY_MONITOR_EXEC";
	fi    
	;;


    *)
	echo CMD $CMD not expected
	;;
esac
