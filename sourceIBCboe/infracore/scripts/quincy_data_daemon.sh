#!/bin/bash

USAGE1="$0 CMD[START/STOP] ACCELERATOR[ONLOAD/VMA/OFF] AFFINITY[AF/NF] ";

if [ $# -ne 3 ] ;
then 
    echo $USAGE1;
    exit;
fi

QUINCY_EXEC=$HOME/LiveExec/bin/quincyGlobexDD 
QUINCY_PRODUCT_LIST="6E_0 6J_0 6A_0 6B_0 6C_0 6M_0 6N_0 6S_0"
QUINCY_MCAST_SOURCE="239.229.4.9"
QUINCY_CONFIG="CME_AUR"
QUINCY_CONFIG_DIR=/spare/local/files/QUINCY/

AFFIN_EXEC=$HOME/LiveExec/bin/cpu_affinity_mgr

CMD=$1;
ACR=$2;
AFO=$3;

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

    export VMA_MEM_ALLOC_TYPE=2; export VMA_FORK=1;
    export VMA_TRACELEVEL=3;
    export VMA_LOG_DETAILS=3;
    export VMA_LOG_FILE=/spare/local/logs/alllogs/QUINCY_DEBUG.log
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

PIDDIR=/spare/local/logs/EXEC_PID_DIR ;

mkdir -p $PIDDIR ;

PIDFILE=$PIDDIR/QUINCY"_PIDfile.txt"
QUINCY_OUT_FILE=$HOME/quincy_data_daemon_out.txt 

if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
        # print error & exit
            echo "Cannot start an instance of $QUINCY_EXEC for $QUINCY_CONFIG $QUINCY_CONFIG_DIR $QUINCY_MCAST_SOURCE $QUINCY_PRODUCT_LIST" ;
	else
        # Appending the mds logger pid to the same file -- 

            if [ $ACR == "ONLOAD" ] 
            then
		
		onload $QUINCY_EXEC $QUINCY_CONFIG $QUINCY_CONFIG_DIR $QUINCY_MCAST_SOURCE $QUINCY_PRODUCT_LIST > $QUINCY_OUT_FILE 2>/dev/null &

            else

	        if [ $ACR == "VMA" ]
		then

		    LD_PRELOAD=libvma.so $QUINCY_EXEC $QUINCY_CONFIG $QUINCY_CONFIG_DIR $QUINCY_MCAST_SOURCE $QUINCY_PRODUCT_LIST > $QUINCY_OUT_FILE 2>/dev/null &

		else

		   $QUINCY_EXEC $QUINCY_CONFIG $QUINCY_CONFIG_DIR $QUINCY_MCAST_SOURCE $QUINCY_PRODUCT_LIST > $QUINCY_OUT_FILE 2>/dev/null &

	       fi

            fi  

            MDSLOGGERPID=$!
            echo $MDSLOGGERPID > $PIDFILE

	    if [ $AFO == "AF" ]
	    then

		echo $MDSLOGGERPID "QUINCYDD" >> $AFFINED_PID_PROC

		if [ -f $AFFIN_EXEC ]
		then

	        # Assign affinity to this exec.
		    $AFFIN_EXEC ASSIGN $MDSLOGGERPID > /home/dvcinfra/quincy.COUT.CERR ;

		fi
	    fi

	fi
	;;

    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
            # Stop the mds_logger -- 
	    MDSLOGGERPID=`tail -1 $PIDFILE`
	    kill -2 $MDSLOGGERPID
	    sleep 1;

	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then 
		echo "patience ... "; 
		sleep 4 ; 

		running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    echo "sending SIGKILL" ;
		    kill -9 $MDSLOGGERPID
		fi
	    fi

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $QUINCY_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT since $PIDFILE does not exist"
	fi    
	;;

    force_start|FORCE_START)
	if [ -f $PIDFILE ] ;
	then
	    MDSLOGGERPID=`tail -1 $PIDFILE`;
	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then
   	        # print error & exit
		echo "Cannot start an instance of $QUINCY_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT since $PIDFILE exists and running_proc_string = $running_proc_string"
	    else
                # Appending the mds logger pid to the same file -- 
	        $QUINCY_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT &
	        MDSLOGGERPID=$!
	        echo $MDSLOGGERPID > $PIDFILE
	    fi
	else
            # Appending the mds logger pid to the same file -- 
	    $QUINCY_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT &
	    MDSLOGGERPID=$!
	    echo $MDSLOGGERPID > $PIDFILE
	fi    
	;;

    force_stop|FORCE_STOP)
	if [ -f $PIDFILE ] ;
	then
            # Stop the mds_logger -- 
	    MDSLOGGERPID=`tail -1 $PIDFILE`
	    kill -2 $MDSLOGGERPID
	    sleep 1;
	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $MDSLOGGERPID
	    fi

	    rm -f $PIDFILE
	else
	    # search for pid and stop
            # Stop the mds_logger -- 
	    MDSLOGGERPIDLIST=`ps -efH | grep "$QUINCY_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT" | grep -v grep | awk '{printf "%s ", $2}'`
	    kill -2 $MDSLOGGERPIDLIST
	    sleep 1;
	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $MDSLOGGERPIDLIST
	    fi
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
