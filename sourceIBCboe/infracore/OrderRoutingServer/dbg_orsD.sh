#!/bin/bash

USAGE="$0 EXCH PROFILE CMD";
if [ $# -ne 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

ORS_EXEC=$HOME/infracore_install/bindebug/cme_ilink_ors
#ORS_EXEC=$HOME/infracore/OrderRoutingServer/install-bin/cme_ilink_ors # profiler version

EXCH=$1; shift;
PROFILE=$1; shift;
CMD=$1; shift;

CONFIGDIR=$HOME/infracore_install/Configs/OrderRoutingServer/cfg ;
OUTPUTLOGDIR=/spare/local/ORSlogs/$EXCH/$PROFILE/ ;
PIDDIR=/spare/local/ORSlogs/PID_ORS_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$PROFILE"_"PIDfile.txt

if [ $PWD != $HOME ] ; then cd $HOME ; fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

# echo "EXCH:" $EXCH "PROFILE:" $PROFILE "CMD:" $CMD;

CONFIGFILE=$CONFIGDIR/$PROFILE/ors.cfg #note this need not be the same as $PROFILE. We could do a switch case like even if EXCH iS "CME" and PROFILE is "TEST" CONFIGFILE uses "8Q3"

case $EXCH in
    cme|CME)
	case $PROFILE in
	    *)
		CONFIGFILE=$CONFIGDIR/$PROFILE/ors.cfg #note this need not be the same as $PROFILE. We could do a switch case like even if EXCH iS "CME" and PROFILE is "TEST" CONFIGFILE uses "8Q3"
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
	ORS_EXEC=$HOME/infracore_install/bindebug/TMXATR
	;;
    tmxlopr|TMXLOPR)    
	;;

    *)    
	echo You did not chose CME, EUREX, TMX or BMF
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
		echo "patience ... "; 
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

