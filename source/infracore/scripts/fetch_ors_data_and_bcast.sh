#!/bin/bash

USAGE="$0 MODE CMD(START/STOP) IP PORT";
if [ $# -lt 4 ] ;
then
    echo $USAGE;
    exit;
fi
ACR=$1;
CMD=$2;
MCAST_IP=$3;
MCAST_PORT=$4;


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
    export VMA_LOG_FILE=/spare/local/logs/alllogs/NEWORSMSG_BCASTER.log
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="ORS";

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/OAPI19.1/ ;

ORS_BCASTER_EXEC=$HOME/fetch_ors_reply_and_bcast
PIDDIR=/spare/local/FETCH_ORS_BCASTER ;
PIDFILE=$PIDDIR/FETCH_ORS_BCASTER"_PIDfile.txt";

case $CMD in
    start|START)
        if [ -f $PIDFILE ] ;
        then
        # print error & exit
            echo "Cannot start an instance of $ORS_BCASTER_EXEC since $PIDFILE exists"
        else
        # Appending the mds logger pid to the same file -- 

            if [ "$ONL" = "ON" ]
            then

                onload $ORS_BCASTER_EXEC $MCAST_IP $MCAST_PORT &

            else

                if [ "$ONL" == "VMA" ]
                then

                  LD_PRELOAD=libvma.so $ORS_BCASTER_EXEC $MCAST_IP $MCAST_PORT &

                else

                  $ORS_BCASTER_EXEC $MCAST_IP $MCAST_PORT &

                fi

            fi

            ORSBCASTERPID=$!
            echo $ORSBCASTERPID > $PIDFILE
	fi
	;;

    stop|STOP)
        if [ -f $PIDFILE ] ;
        then
            # Stop the mds_logger -- 
            ORSBCASTERPID=`tail -1 $PIDFILE`
            kill -2 $ORSBCASTERPID
            sleep 1;

            running_proc_string=`ps -p $ORSBCASTERPID -o comm=`;
            if [ $running_proc_string ] ; then
                echo "patience ... ";
                sleep 4 ;

                running_proc_string=`ps -p $ORSBCASTERPID -o comm=`;
                if [ $running_proc_string ] ; then
                # by now if still runing SIGINT did not work, sending SIGKILL
                    echo "sending SIGKILL" ;
                    kill -9 $ORSBCASTERPID
                fi
            fi

            rm -f $PIDFILE
        else
            # print error & exit
            echo "Cannot stop an instance of $ORS_BCASTER_EXEC since $PIDFILE does not exist"
        fi
        ;;

    force_start|FORCE_START)
        if [ -f $PIDFILE ] ;
        then
            ORSBCASTERPID=`tail -1 $PIDFILE`;
            running_proc_string=`ps -p $ORSBCASTERPID -o comm=`;
            if [ $running_proc_string ] ; then
                # print error & exit
                echo "Cannot start an instance of $ORS_BCASTER_EXEC $MCAST_IP $MCAST_PORT since $PIDFILE exists and running_proc_string = $running_proc_string"
            else
                # Appending the mds logger pid to the same file -- 
                $ORS_BCASTER_EXEC $MCAST_IP $MCAST_PORT &
                ORSBCASTERPID=$!
                echo $ORSBCASTERPID > $PIDFILE
            fi
        else
            # Appending the mds logger pid to the same file -- 
            $ORS_BCASTER_EXEC $MCAST_IP $MCAST_PORT &
            ORSBCASTERPID=$!
            echo $ORSBCASTERPID > $PIDFILE
        fi
        ;;

    force_stop|FORCE_STOP)
        if [ -f $PIDFILE ] ;
        then
            # Stop the mds_logger -- 
            ORSBCASTERPID=`tail -1 $PIDFILE`
            kill -2 $ORSBCASTERPID
            sleep 1;
            running_proc_string=`ps -p $ORSBCASTERPID -o comm=`;
            if [ $running_proc_string ] ; then
                kill -9 $ORSBCASTERPID
            fi

            rm -f $PIDFILE
        else
            # search for pid and stop
            # Stop the mds_logger -- 
            ORSBCASTERPIDLIST=`ps -efH | grep "$ORS_BCASTER_EXEC $MCAST_IP $MCAST_PORT" | grep -v grep | awk '{printf "%s ", $2}'`
            kill -2 $ORSBCASTERPIDLIST
            sleep 1;
            running_proc_string=`ps -p $ORSBCASTERPID -o comm=`;
            if [ $running_proc_string ] ; then
                kill -9 $ORSBCASTERPIDLIST
            fi
        fi
        ;;
    *)
	echo CMD $CMD not expected
	;;
esac
