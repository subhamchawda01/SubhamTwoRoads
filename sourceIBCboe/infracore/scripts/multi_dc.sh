#!/bin/bash
#Script to start BMFMultiDropcopy (exec handling multiple DC sessions simultaneously)
USAGE="$0 EXCH LOG_PROFILE CMD ACCELERATOR[ONLOAD(ON)/VMA/OFF] DROPCOPY_PROFILE_1 [DROPCOPY_PROFILE_2 ...]";

if [ $# -le 5 ] ;
then
    echo $USAGE;
    exit;
fi

MULT_DC_EXEC=$HOME/LiveExec/bin/BMFMultiDropcopy

EXCH=$1; shift;
LOG_PROFILE=$1; shift;
CMD=$1; shift;
ACR=$1; shift;

CONFIGDIR=$HOME/infracore_install/Configs/OrderRoutingServer/cfg ;
OUTPUTLOGDIR=/spare/local/ORSlogs/$EXCH/$LOG_PROFILE/ ;
PIDDIR=/spare/local/ORSlogs/PID_ORS_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$LOG_PROFILE"_"PIDfile.txt

logfile=$OUTPUTLOGDIR"log.`date "+%Y%m%d"`"
positionfile=$OUTPUTLOGDIR"position.`date "+%Y%m%d"`"
tradesfile=$OUTPUTLOGDIR"trades.`date "+%Y%m%d"`"

COUT_CERR_FILE=$OUTPUTLOGDIR/COUT_CERR.log

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

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $0 since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi

            #this file is opened in append mode, but still making a copy
            if [ -f $tradesfile ] ;
            then
                cp $tradesfile $tradesfile"_`date "+%s"`"
            fi
            
            config_string=""
            #Generate config string
            for dc_profile in $*;
            do
              config_string=$config_string" --config $CONFIGDIR/$dc_profile/ors.cfg"
            done

            if [ "$ACR" == "ONLOAD" ] || [ "$ACR" == "ON" ]
            then

    	        onload $MULT_DC_EXEC $config_string --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE 2>&1 &

            else

	        if [ "$ACR" == "VMA" ]
		then

		    #dynamically linked lib
      	            LD_PRELOAD=libvma.so $MULT_DC_EXEC $config_string --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE 2>&1 & 

		else

      	            $MULT_DC_EXEC $config_string --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE 2>&1 &

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
	    sleep 30;

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
	    echo "Cannot stop an instance of $0 since $PIDFILE does not exist"
	fi

	;;
    *)
	echo CMD $CMD not expected
	;;
esac

