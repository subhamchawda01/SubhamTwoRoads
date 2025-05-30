#!/bin/bash

USAGE1="$0 CMD[START/STOP] ACCELERATOR[ONLOAD/VMA/OFF] AFFINITY[AF/NF] ";

if [ $# -ne 3 ] ;
then
    echo $USAGE1;
    exit;
fi

VOLMON_EXEC=/home/pengine/prod/live_execs/volume_monitor
VOLMON_SHORTCODE_LIST=/spare/local/files/oebu_volmon_product_list.txt
VOLMON_OUT_FILE=$HOME/moving_volumes.txt

AFFIN_EXEC=/home/pengine/prod/live_execs/cpu_affinity_mgr

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
    export VMA_LOG_FILE=/spare/local/logs/alllogs/VOLMON_DEBUG.log
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="MDS"

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64

if [ `hostname | grep "TOK" | wc -l` -gt 0 ]  #TOK server
then

   export TZ="Asia/Tokyo" ;

fi


## AFFINED_PID_FILE
AFFINED_PID_PROC=/spare/local/files/affinity_pid_process.txt ;

PIDDIR=/spare/local/logs/EXEC_PID_DIR ;

PIDFILE=$PIDDIR/VOLMON"_PIDfile.txt"

if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
        # print error & exit
            echo "Cannot start an instance of $VOLMON_EXEC for $VOLMON_SHORTCODE_LIST" ;
	else
        # Appending the mds logger pid to the same file --

            if [ $ACR == "ONLOAD" ]
            then

		onload $VOLMON_EXEC $VOLMON_SHORTCODE_LIST > $VOLMON_OUT_FILE 2>&1 &

            else

	        if [ $ACR == "VMA" ]
		then

		    LD_PRELOAD=libvma.so $VOLMON_EXEC $VOLMON_SHORTCODE_LIST > $VOLMON_OUT_FILE 2>&1 &

		else

		   $VOLMON_EXEC $VOLMON_SHORTCODE_LIST > $VOLMON_OUT_FILE 2>&1 &

	       fi

            fi

            MDSLOGGERPID=$!
            echo $MDSLOGGERPID > $PIDFILE

	    if [ $AFO == "AF" ]
	    then

		echo $MDSLOGGERPID $EXCH"VOLMON" >> $AFFINED_PID_PROC

		if [ -f $AFFIN_EXEC ]
		then

	        # Assign affinity to this exec.
		    $AFFIN_EXEC ASSIGN $MDSLOGGERPID > /home/dvcinfra/oebu.COUT.CERR ;

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
	    echo "Cannot stop an instance of $VOLMON_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT since $PIDFILE does not exist"
	fi
	;;

    force_start|FORCE_START)
	if [ -f $PIDFILE ] ;
	then
	    MDSLOGGERPID=`tail -1 $PIDFILE`;
	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then
   	        # print error & exit
		echo "Cannot start an instance of $VOLMON_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT since $PIDFILE exists and running_proc_string = $running_proc_string"
	    else
                # Appending the mds logger pid to the same file --
	        $VOLMON_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT &
	        MDSLOGGERPID=$!
	        echo $MDSLOGGERPID > $PIDFILE
	    fi
	else
            # Appending the mds logger pid to the same file --
	    $VOLMON_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT &
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
	    MDSLOGGERPIDLIST=`ps -efH | grep "$VOLMON_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT" | grep -v grep | awk '{printf "%s ", $2}'`
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

if [ `hostname | grep "TOK" | wc -l` -gt 0 ]  #TOK server
then

   export TZ="Etc/UTC" ;

fi

