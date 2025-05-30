#!/bin/bash

USAGE1="$0 EXCH LOC YYYYMMDD CMD[START/STOP] ONLOAD[ON/OFF] AFFINITY[AF/NF] <- LOC & YYYYMMDD unused.";
USAGE2="$0 EXCH LOC YYYYMMDD CMD[COPYDATA/CLEARDATA] ON NF";

if [ $# -ne 6 ] ; 
then 
    echo $USAGE1;
    echo $USAGE2;
    exit;
fi

ORSBCAST_LOG_EXEC=$HOME/LiveExec/bin/ors_binary_logger
AFFIN_EXEC=$HOME/LiveExec/bin/cpu_affinity_mgr

COPY_LOG_EXEC=$HOME/LiveExec/scripts/ors_binary_log_backup.sh
CLEAR_LOG_EXEC=$HOME/LiveExec/scripts/ors_binary_clear_log.sh

EXCH=$1;
LOC=$2;
YYYYMMDD=$3;
CMD=$4;
ONL=$5;
AFO=$6;

if [ "$ONL" == "ONLOAD" ] || [ "$ONL" == "ON" ]
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

if [ "$ONL" == "VMA" ]
then

    export VMA_MEM_ALLOC_TYPE=2; export VMA_FORK=1;
    export VMA_TRACELEVEL=3;
    export VMA_LOG_DETAILS=3;
    export VMA_LOG_FILE=/spare/local/logs/alllogs/MDSLOGGER_DEBUG.log
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="MDS"

fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


## AFFINED_PID_FILE
AFFINED_PID_PROC=/spare/local/files/affinity_pid_process.txt ;

PIDDIR=/spare/local/ORSBCAST/PID_ORSBCAST_DIR ;
mkdir -p $PIDDIR

PIDFILE=$PIDDIR/ORSBCASTLOGGER_$EXCH"_PIDfile.txt"

if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $EXCH in
    ALL)
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

            if [ $ONL = "ON" ] 
            then

	        onload $ORSBCAST_LOG_EXEC --exch $EXCH &

	    else

                if [ "$ONL" == "VMA" ]
                then 

                  LD_PRELOAD=libvma.so $ORSBCAST_LOG_EXEC --exch $EXCH &

                else 

                  $ORSBCAST_LOG_EXEC --exch $EXCH &

                fi 

	    fi

	    ORSBCASTLOGGERPID=$!
	    echo $ORSBCASTLOGGERPID > $PIDFILE

            if [ $AFO = "AF" ]
            then

                echo $ORSBCASTLOGGERPID $EXCH"ORSBinaryLogger" >> $AFFINED_PID_PROC

                if [ -f $AFFIN_EXEC ] ;
                then
                    # Assign affinity to this exec.
                    $AFFIN_EXEC ASSIGN $ORSBCASTLOGGERPID > /home/dvcinfra/orsbinlogger.COUT.CERR ;
                fi

            fi  

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
