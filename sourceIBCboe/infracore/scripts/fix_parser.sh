#!/bin/bash

USAGE="$0 EXCH ALPES_PROFILE ICAP_PROFILE CMD ACCELERATOR[ONLOAD(ON)/VMA/OFF] POSFILE[KEEP/CLEAR]";

if [ $# -ne 6 ] ;
then
    echo $USAGE;
    exit;
fi

ORS_EXEC=$HOME/LiveExec/bin/alpes_gui_fix_parser

EXCH=$1; shift;
ALPES_PROFILE=$1; shift;
ICAP_PROFILE=$1 ; shift;
CMD=$1; shift;
ACR=$1; shift;
POS=$1; shift;

CONFIGDIR=$HOME/infracore_install/Configs/OrderRoutingServer/cfg ;
OUTPUTLOGDIR=/spare/local/ORSlogs/$EXCH/RETAIL
PIDDIR=/spare/local/ORSlogs/PID_ORS_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$ALPES_PROFILE"_"$ICAP_PROFILE"_"PIDfile.txt

logfile=$OUTPUTLOGDIR"log.`date "+%Y%m%d"`"
positionfile=$OUTPUTLOGDIR"position.`date "+%Y%m%d"`"
tradesfile=$OUTPUTLOGDIR"trades.`date "+%Y%m%d"`"

COUT_CERR_FILE=/spare/local/logs/alllogs/ALPES_FIX_COUT_CERR.log

if [ "$ACR" == "ONLOAD" ] || [ "$ACR" == "ON" ]
then

    export EF_LOG_VIA_IOCTL=1 ;
    export EF_NO_FAIL=0 ;
    export EF_UDP_RECV_MCAST_UL_ONLY=1 ;
    export EF_SPIN_USEC=-1 ; export EF_POLL_USEC=-1 ; export EF_SELECT_SPIN=1 ;
    export EF_MULTICAST_LOOP_OFF=0 ;
    export EF_MAX_ENDPOINTS=1024 ;
    export EF_SHARE_WITH=-1;
    export EF_NAME=ORS_STACK ;

fi

if [ "$ACR" == "VMA" ]
then

    export VMA_CONFIG_FILE=/spare/local/files/VMA/libvma.conf
    export VMA_MEM_ALLOC_TYPE=2; export VMA_FORK=1;
    export VMA_TRACELEVEL=3;
    export VMA_LOG_DETAILS=3;
    export VMA_LOG_FILE=/spare/local/logs/alllogs/ORS_DEBUG.log
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="ORS";

fi


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/OAPI19.1/ ;

if [ $PWD != $HOME ] ; then cd $HOME ; fi

# echo "EXCH:" $EXCH "PROFILE:" $PROFILE "CMD:" $CMD;

ALPES_CONFIGFILE=$CONFIGDIR/$ALPES_PROFILE/ors.cfg #note this need not be the same as $PROFILE. We could do a switch case like even if EXCH iS "CME" and PROFILE is "TEST" CONFIGFILE uses "8Q3"
ICAP_CONFIGFILE=$CONFIGDIR/$ICAP_PROFILE/ors.cfg


case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $ORS_EXEC $ALPES_CONFIGFILE $ICAP_CONFIGFILE $OUTPUTLOGDIR since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi

            #this file is opened in append mode, but still making a copy
            if [ -f $tradesfile ] ;
            then
                cp $tradesfile $tradesfile"_`date "+%s"`"
            fi

            if [ "$ACR" == "ONLOAD" ] || [ "$ACR" == "ON" ]
            then

    	        onload $ORS_EXEC --alpes-config $ALPES_CONFIGFILE --icap-config $ICAP_CONFIGFILE --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE >>$COUT_CERR_FILE &

            else

	        if [ "$ACR" == "VMA" ]
		then

		    #dynamically linked lib
      	            LD_PRELOAD=libvma.so $ORS_EXEC --alpes-config $ALPES_CONFIGFILE --icap-config $ICAP_CONFIGFILE --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE >>$COUT_CERR_FILE &

		else

      	            $ORS_EXEC --alpes-config $ALPES_CONFIGFILE --icap-config $ICAP_CONFIGFILE --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE >>$COUT_CERR_FILE &

		fi

            fi

	    ORSPID=$!
	    echo $ORSPID > $PIDFILE
	fi
	;;
    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
	    ORSPID=`tail -1 $PIDFILE`
	    kill -2 $ORSPID 2>/dev/null
	    sleep 1;

	    running_proc_string=`ps -p $ORSPID -o comm=`;
	    if [ $running_proc_string ] ; then
#		echo "patience ... "; # removed to avoid the email in crontab
		sleep 15 ;

		running_proc_string=`ps -p $ORSPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    echo "sending SIGKILL" ;
		    kill -9 $ORSPID 2>/dev/null
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
	    kill -2 $ORSPID 2>/dev/null
	    sleep 1;
	    running_proc_string=`ps -p $ORSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $ORSPID 2>/dev/null
	    fi
	    rm -f $PIDFILE
	else
	    # search for pid and stop
	    ORSPIDLIST=`ps -efH | grep "$ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR" | grep -v grep | awk '{printf "%s ", $2}'`
	    kill -2 $ORSPIDLIST 2>/dev/null
	    sleep 15;
	    running_proc_string=`ps -p $ORSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $ORSPIDLIST 2>/dev/null
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

if [ "$EXCH" == "OSE" ]
then

    export TZ="Etc/UTC" ;   # We are done with ors start/stop, revert back

fi

