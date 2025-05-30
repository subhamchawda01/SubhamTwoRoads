#!/bin/bash

#only meant for chix and eurex for now 
USAGE1="$0 EXCH CMD[START/STOP] ONLOAD[ON/OFF] AFFINITY[AF/NF]";

if [ $# -ne 4 ] ;
then 
    echo $USAGE1;
    echo $USAGE2;
    exit;
fi

MDS_LOG_EXEC=$HOME/LiveExec/bin/fetch_hkex_shm_data_and_bcast

AFFIN_EXEC=$HOME/LiveExec/bin/cpu_affinity_mgr

COPY_LOG_EXEC=$HOME/LiveExec/scripts/mds_log_backup.sh
CLEAR_LOG_EXEC=$HOME/LiveExec/scripts/mds_log_clear.sh

EXCH=$1;
CMD=$2;
ONL=$3;
AFO=$4;

if [ "$ONL" == "ONLOAD" ] || [ "$ONL" == "ON" ]
then

    export EF_LOG_VIA_IOCTL=1 ;
    export EF_NO_FAIL=0 ;
    export EF_UDP_RECV_MCAST_UL_ONLY=1 ;
    export EF_SPIN_USEC=100000 ; export EF_POLL_USEC=100000 ; export EF_SELECT_SPIN=1 ;
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
    export VMA_LOG_FILE=/spare/local/logs/alllogs/MDSSHMLOGGER_DEBUG.log
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="MDS"

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

if [ "$EXCH" == "HKEX" ] 
then 

   MDS_LOG_EXEC=$HOME/LiveExec/bin/fetch_hkex_shm_data_and_bcast 

fi 

if [ "$EXCH" == "TMX" ] 
then 

   MDS_LOG_EXEC=$HOME/LiveExec/bin/fetch_mx_shm_data_and_bcast 

fi 

if [ "$EXCH" == "OSE_L1" ] 
then 

#   MDS_LOG_EXEC=$HOME/LiveExec/bin/ose_fetch_order_data_from_shm_and_bcast_l1 
   MDS_LOG_EXEC=$HOME/LiveExec/bin/ose_l1_data_generator_via_shm_pricefeed 

fi 

if [ "$EXCH" == "OSEPriceFeed" ] 
then 

   MDS_LOG_EXEC=$HOME/LiveExec/bin/fetch_ose_pricefeed_data_and_bcast 

fi 

if [ "$EXCH" == "TSE" ] 
then 

   MDS_LOG_EXEC=$HOME/LiveExec/bin/fetch_tse_data_and_bcast

fi 


## AFFINED_PID_FILE
AFFINED_PID_PROC=/spare/local/files/affinity_pid_process.txt ;

PIDDIR=/spare/local/MDSlogs/PID_MDS_DIR ;

PIDFILE=$PIDDIR/FETCH_SHM_BCAST_$EXCH"_PIDfile.txt"

if [ $PWD != $HOME ] ; then cd $HOME ; fi

EXCH_TMP=$EXCH
nw_file="$HOME/infracore_install/SysInfo/TradingInfo/NetworkInfo/network_account_info_filename.txt"
host=`hostname`

MCAST_IP=`grep $host $nw_file | grep "$EXCH_TMP" | awk '{print $4}'`
MCAST_PORT=`grep $host $nw_file | grep "$EXCH_TMP" | awk '{print $5}'`

if [[ "$MCAST_PORT" != +([0-9]) ]] ; then MCAST_PORT=12345 ; MCAST_IP="127.0.0.1" ; fi


if [ "$CMD" == "START" ]
then

if [ $MCAST_IP == "127.0.0.1" ]
then

    echo "N/w Info Invalid" ;
    echo "Data Logger" $EXCH `hostname` " Got Loopback Address" | /bin/mail -s "Data Daemon Invalid IP/PORT" -r "networkrefinfo" "nseall@tworoads.co.in" ;
    exit ;

fi

fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
        # print error & exit
            echo "Cannot start an instance of $MDS_LOG_EXEC since $PIDFILE exists"
	else
        # Appending the mds logger pid to the same file -- 

            if [ $ONL = "ON" ] 
            then
		
		onload $MDS_LOG_EXEC $MCAST_IP $MCAST_PORT &

            else

                if [ "$ONL" == "VMA" ] 
                then 

                  LD_PRELOAD=libvma.so $MDS_LOG_EXEC $MCAST_IP $MCAST_PORT & 

                else 

                  $MDS_LOG_EXEC $MCAST_IP $MCAST_PORT &

                fi 

            fi  

            MDSLOGGERPID=$!
            echo $MDSLOGGERPID > $PIDFILE

	    if [ $AFO = "AF" ]
	    then

		echo $MDSLOGGERPID $EXCH"DataLogger" >> $AFFINED_PID_PROC

		if [ -f $AFFIN_EXEC ]
		then

	        # Assign affinity to this exec.
		    $AFFIN_EXEC ASSIGN $MDSLOGGERPID > /home/dvcinfra/mdslogger.COUT.CERR ;

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
	    echo "Cannot stop an instance of $MDS_LOG_EXEC since $PIDFILE does not exist"
	fi    
	;;

    force_start|FORCE_START)
	if [ -f $PIDFILE ] ;
	then
	    MDSLOGGERPID=`tail -1 $PIDFILE`;
	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then
   	        # print error & exit
		echo "Cannot start an instance of $MDS_LOG_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT since $PIDFILE exists and running_proc_string = $running_proc_string"
	    else
                # Appending the mds logger pid to the same file -- 
	        $MDS_LOG_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT &
	        MDSLOGGERPID=$!
	        echo $MDSLOGGERPID > $PIDFILE
	    fi
	else
            # Appending the mds logger pid to the same file -- 
	    $MDS_LOG_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT &
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
	    MDSLOGGERPIDLIST=`ps -efH | grep "$MDS_LOG_EXEC --exch $EXCH --ip $MCAST_IP --port $MCAST_PORT" | grep -v grep | awk '{printf "%s ", $2}'`
	    kill -2 $MDSLOGGERPIDLIST
	    sleep 1;
	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $MDSLOGGERPIDLIST
	    fi
	fi    
	;;


    *)
	echo CMD $CMD not expected
	;;
esac
