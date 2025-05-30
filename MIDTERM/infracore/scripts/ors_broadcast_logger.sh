#!/bin/bash

USAGE1="$0 EXCH CMD[START/STOP] ONLOAD[ON/OFF] AFFINITY[AF/NF]";

if [ $# -ne 4 ] ; 
then 
    echo $USAGE1;
    exit;
fi

ORSBCAST_LOG_EXEC="/home/pengine/prod/live_execs/ors_broadcast_logger"

AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr"


EXCH=$1;
CMD=$2;
ONL=$3;
AFO=$4;


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

    export VMA_CONFIG_FILE=/spare/local/files/VMA/libvma.conf 
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

PIDDIR=/spare/local/ORSBCAST_LOGGER_PID/PID_ORSBCAST_DIR ;
mkdir -p $PIDDIR

PIDFILE=$PIDDIR/ORSBCASTLOGGER_$EXCH"_PIDfile.txt"

## AFFINED_PID_FILE
AFFINED_PID_PROC=/spare/local/files/affinity_pid_process.txt ;

if [ $PWD != $HOME ] ; then cd $HOME ; fi

ORSBCAST_IP=127.0.0.1
ORSBCAST_PORT=12345

nw_file="/home/pengine/prod/live_configs/"`hostname`"_network_account_info_filename.cfg"
ORSBCAST_IP=`grep "EXCHTRADE" $nw_file | grep "$EXCH " | awk '{print $6}'`
ORSBCAST_PORT=`grep "EXCHTRADE" $nw_file | grep "$EXCH " | awk '{print $7}'`

if [[ "$ORSBCAST_PORT" != +([0-9]) ]] ; then ORSBCAST_PORT=12345 ; ORSBCAST_IP="127.0.0.1" ; fi

if [ $ORSBCAST_IP = "127.0.0.1" ]
then

    echo "N/w Info Invalid" ;
    echo "ORS Binary Logger @ " `hostname` " Got Loopback Address" | /bin/mail -s "Data Daemon Invalid IP/PORT" -r "networkrefinfo" "nseall@tworoads.co.in" ;
    exit ;

fi

case $EXCH in
    CME)
	;;
    
    EUREX)
	;;

    TMX)
	;;
    
    BMF)
	;;
    LIFFE)
	;;
    
    HONGKONG)
	;;

    OSE)
	;;

    SGX)
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
		
	        onload $ORSBCAST_LOG_EXEC $ORSBCAST_IP $ORSBCAST_PORT &

	    else

                if [ "$ONL" == "VMA" ]
                then

                  LD_PRELOAD=libvma.so $ORSBCAST_LOG_EXEC $ORSBCAST_IP $ORSBCAST_PORT &

                else 

                  $ORSBCAST_LOG_EXEC $ORSBCAST_IP $ORSBCAST_PORT &

                fi 

	    fi

	    ORSBCASTLOGGERPID=$!
	    echo $ORSBCASTLOGGERPID > $PIDFILE

            if [ $AFO = "AF" ]
            then

                echo $ORSBCASTLOGGERPID $EXCH"ORSBroadcastLogger" >> $AFFINED_PID_PROC

                if [ -f $AFFIN_EXEC ] ;
                then
                    # Assign affinity to this exec.
                    $AFFIN_EXEC ASSIGN $ORSBCASTLOGGERPID > /home/dvcinfra/orsbcastlogger.COUT.CERR ;
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

    *)
	echo CMD $CMD not expected
	;;
esac
