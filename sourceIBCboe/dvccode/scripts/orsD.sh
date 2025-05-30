#!/bin/bash

USAGE="$0 EXCH PROFILE CMD";
if [ $# -ne 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

ORS_EXEC=/home/pengine/prod/live_execs/cme_ilink_ors

EXCH=$1; shift;
PROFILE=$1; shift;
CMD=$1; shift;

OUTPUTLOGDIR=/spare/local/ORSlogs/$EXCH/$PROFILE/ ;
PIDDIR=/spare/local/ORSlogs/PID_ORS_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$PROFILE"_"PIDfile.txt

logfile=$OUTPUTLOGDIR"log.`date "+%Y%m%d"`"
positionfile=$OUTPUTLOGDIR"position.`date "+%Y%m%d"`"
tradesfile=$OUTPUTLOGDIR"trades.`date "+%Y%m%d"`"

if [ $PWD != $HOME ] ; then cd $HOME ; fi

# echo "EXCH:" $EXCH "PROFILE:" $PROFILE "CMD:" $CMD;


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/OAPI19.1/ ;

CONFIGFILE="/home/pengine/prod/live_configs/common_${PROFILE}_ors.cfg" #note this need not be the same as $PROFILE. We could do a switch case like even if EXCH iS "CME" and PROFILE is "TEST" CONFIGFILE uses "8Q3"

case $EXCH in
    cme|CME)
	case $PROFILE in
	    *)
		CONFIGFILE="/home/pengine/prod/live_configs/common_${PROFILE}_ors.cfg" #note this need not be the same as $PROFILE. We could do a switch case like even if EXCH iS "CME" and PROFILE is "TEST" CONFIGFILE uses "8Q3"
		;;
	esac
	;;
    eurex|EUREX)    
	;;
    tmx|TMX)    
	;;
    bmf|BMF)    
	;;
    bmfep|BMFEP)    
	;;
    tmxatr|TMXATR)
	ORS_EXEC=/home/pengine/prod/live_execs/TMXATR
	;;
    tmxlopr|TMXLOPR)    
	;;

    *)    
	echo You did not chose CME, EUREX, TMX, TMXATR, BMF or TMXLOPR
	;;
esac

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $ORS_EXEC $CONFIGFILE $OUTPUTLOGDIR since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi

            # backup log files if already exists, using timestamp to allow multiple backups
            if [ -f $logfile ] ;
            then
                cp $logfile $logfile"_`date "+%s"`"
            fi

            if [ -f $positionfile ] ;
            then
                cp $positionfile $positionfile"_`date "+%s"`"
            fi

            #this file is opened in append mode, but still making a copy 
            if [ -f $tradesfile ] ;
            then
                cp $tradesfile $tradesfile"_`date "+%s"`"
            fi

	    $ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR &
	    ORSPID=$!
	    echo $ORSPID > $PIDFILE
	fi    
	;;
    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
	    ORSPID=`tail -1 $PIDFILE`
	    kill -2 $ORSPID
	    sleep 1;

	    running_proc_string=`ps -p $ORSPID -o comm=`;
	    if [ $running_proc_string ] ; then 
#		echo "patience ... "; # removed to avoid the email in crontab
		sleep 10 ; 

		running_proc_string=`ps -p $ORSPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    echo "sending SIGKILL" ;
		    kill -9 $ORSPID
		fi
	    fi

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $ORS_EXEC $CONFIGFILE $OUTPUTLOGDIR since $PIDFILE does not exist"
	fi    

	;;
    force_start|FORCE_START)
	if [ -f $PIDFILE ] ;
	then
	    ORSPID=`tail -1 $PIDFILE`;
	    running_proc_string=`ps -p $ORSPID -o comm=`;
	    if [ $running_proc_string ] ; then
   	        # print error & exit
		echo "Cannot start an instance of $ORS_EXEC $CONFIGFILE $OUTPUTLOGDIR since $PIDFILE exists and running_proc_string = $running_proc_string"
	    else
		if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
		if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi
		$ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR &
		ORSPID=$!
		echo $ORSPID > $PIDFILE
	    fi
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi
	    $ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR &
	    ORSPID=$!
	    echo $ORSPID > $PIDFILE
	fi    
	;;
    force_stop|FORCE_STOP)
	if [ -f $PIDFILE ] ;
	then
	    ORSPID=`tail -1 $PIDFILE`
	    kill -2 $ORSPID
	    sleep 1;
	    running_proc_string=`ps -p $ORSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $ORSPID
	    fi
	    rm -f $PIDFILE
	else
	    # search for pid and stop
	    ORSPIDLIST=`ps -efH | grep "$ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR" | grep -v grep | awk '{printf "%s ", $2}'`
	    kill -2 $ORSPIDLIST
	    sleep 10;
	    running_proc_string=`ps -p $ORSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $ORSPIDLIST
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

