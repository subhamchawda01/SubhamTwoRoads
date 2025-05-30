#!/bin/bash

USAGE="$0 CONFIGFILE CMD";
if [ $# -ne 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

LETS_EXEC=$HOME/LiveExec/bin/lets_auction_notifier

CONFIGFILE=$1
CMD=$2 

PIDDIR=/spare/local/LETS
PIDFILE=$PIDDIR/LETS_PIDfile.txt
LOGFILE=/spare/local/LETS/lets_coutcerr.log

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $LETS_EXEC $CONFIGFILE since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi

	    $LETS_EXEC $CONFIGFILE > $LOGFILE &
	    LETSPID=$!
	    echo $LETSPID > $PIDFILE
	fi    
	;;
    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
	    LETSPID=`tail -1 $PIDFILE`
	    kill -2 $LETSPID
	    sleep 1;

	    running_proc_string=`ps -p $LETSPID -o comm=`;
	    if [ $running_proc_string ] ; then 
#		echo "patience ... "; # removed to avoid the email in crontab
		sleep 10 ; 

		running_proc_string=`ps -p $LETSPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    echo "sending SIGKILL" ;
		    kill -9 $LETSPID
		fi
	    fi

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $LETS_EXEC $CONFIGFILE since $PIDFILE does not exist"
	fi    

	;;
    force_start|FORCE_START)
	if [ -f $PIDFILE ] ;
	then
	    LETSPID=`tail -1 $PIDFILE`;
	    running_proc_string=`ps -p $LETSPID -o comm=`;
	    if [ $running_proc_string ] ; then
   	        # print error & exit
		echo "Cannot start an instance of $ORS_EXEC $CONFIGFILE $OUTPUTLOGDIR since $PIDFILE exists and running_proc_string = $running_proc_string"
	    else
		if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
		if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi
		$ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR &
		LETSPID=$!
		echo $LETSPID > $PIDFILE
	    fi
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi
	    $ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR &
	    LETSPID=$!
	    echo $LETSPID > $PIDFILE
	fi    
	;;
    force_stop|FORCE_STOP)
	if [ -f $PIDFILE ] ;
	then
	    LETSPID=`tail -1 $PIDFILE`
	    kill -2 $LETSPID
	    sleep 1;
	    running_proc_string=`ps -p $LETSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $LETSPID
	    fi
	    rm -f $PIDFILE
	else
	    # search for pid and stop
	    LETSPIDLIST=`ps -efH | grep "$ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR" | grep -v grep | awk '{printf "%s ", $2}'`
	    kill -2 $LETSPIDLIST
	    sleep 10;
	    running_proc_string=`ps -p $LETSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $LETSPIDLIST
	    fi
	fi    

	;;
    copylogs|COPYLOGS)

	;;
    clearlogs|CLEARLOGS)

	;;
    *)
	echo CMD $CMD not expected
	;;
esac

