#!/bin/bash

USAGE="$0 CMD ACCELERATOR[ON/VMA/OFF] AFFINITY[AF/NF]";

if [ $# -lt 3 ] ;
then
    echo $USAGE;
    exit;
fi
P2_EXEC=/spare/local/lib/cgate/P2MQRouter
AFFIN_EXEC=$HOME/LiveExec/bin/cpu_affinity_mgr

CMD=$1; shift;
ACR=$1; shift; #onload switch option
AFO=$1; shift;
if [ $# == 1 ] ; then  OPTARG=$1 ; else OPTARG="DUMMY" ; fi ; #optional arguments OPTARG=$1; shift; #optional arguments

COUTCERRFILE="/spare/local/MDSlogs/P2MQRouter.""$YYYYMMDD"_cout_cerr_file.txt;



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
    export VMA_LOG_FILE=/spare/local/logs/alllogs/NEWFIXFAST_DEBUG.log
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="MKTDD";

fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH:/apps/NYLIBSTORE/:/spare/local/lib/cgate;

## AFFINED_PID_FILE
AFFINED_PID_PROC=/spare/local/files/affinity_pid_process.txt ;

OUTPUTLOGDIR=/spare/local/MDSlogs/$EXCH ;
PIDDIR=/spare/local/MDSlogs/PID_MDS_DIR ;
LOGFILE=/spare/local/lib/cgate/log/CLIENT_router.log;
PIDFILE=$PIDDIR/P2MQRouter"_"PIDfile.txt

YYYYMMDD=$(date "+%Y%m%d");
CGATEHOME=/spare/local/lib/cgate

if [ $PWD != $CGATEHOME ] ; then cd $CGATEHOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $MDS_EXEC since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi

            # if the logfile already exists take a backup of the error_stream_log
            # used time with the filename to allow multiple backups
            if [ -f $LOGFILE ] ;
            then
                cp $LOGFILE $LOGFILE"_`date "+%s"`"
            fi


	    if [ $ACR == "ON" ]
              then
	  	onload $P2_EXEC > $LOGFILE >> $COUTCERRFILE &
	      else
	    	if [ $ACR == "VMA" ]
	        then
		    LD_PRELOAD=libvma.so $P2_EXEC >$LOGFILE >> $COUTCERRFILE &
		else
		    $P2_EXEC >$LOGFILE >> $COUTCERRFILE &
		fi
	    fi


	    MDSPID=$!
	    echo $MDSPID > $PIDFILE

	    if [ $AFO == "AF" ]
	    then

                echo $MDSPID $EXCH"DataDaemon" >> $AFFINED_PID_PROC

	        if [ -f $AFFIN_EXEC ] ;
	        then
	            # Assign affinity to this exec.
		    $AFFIN_EXEC ASSIGN $MDSPID >> $COUTCERRFILE ;
	        fi

	    fi

	fi
	;;

    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
	    MDSPID=`tail -1 $PIDFILE`
	    kill -2 $MDSPID
	    sleep 1;

	    running_proc_string=`ps -p $MDSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		echo "patience ... ";
		sleep 4 ;

		running_proc_string=`ps -p $MDSPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    kill -9 $MDSPID
		fi
	    fi

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $P2_EXEC since $PIDFILE does not exist"
	fi
	;;


    force_stop|FORCE_STOP)
	if [ -f $PIDFILE ] ;
	then
	    MDSPID=`tail -1 $PIDFILE`
	    kill -2 $MDSPID
	    sleep 1;
	    running_proc_string=`ps -p $MDSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $MDSPID
	    fi

	    rm -f $PIDFILE
	else
	    # search for pid and stop
	    MDSPIDLIST=`ps -efH | grep "$P2_EXEC" | grep -v grep | awk '{printf "%s ", $2}'`
	    kill -2 $MDSPIDLIST
	    sleep 1;
	    running_proc_string=`ps -p $MDSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $MDSPIDLIST
	    fi
	fi
	;;

    *)
	echo CMD $CMD not expected
	;;
esac

