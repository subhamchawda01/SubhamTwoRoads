#!/bin/bash

USAGE="$0 EXCH MODE CMD ACCELERATOR[ON/VMA/OFF] AFFINITY[AF/NF]";
if [ $# -ne 5 ] ; 
then 
    echo $USAGE;
    exit;
fi

MDS_EXEC=/home/pengine/prod/live_execs/fixfast-mds_MCAST

EXCH=$1; shift;
MODE=$1; shift; #DATA REFERENCE
CMD=$1; shift;
ACR=$1; shift;
AFO=$1; shift;

OUTPUTLOGDIR=/spare/local/MDSlogs/$EXCH ;
PIDDIR=/spare/local/MDSlogs/PID_MDS_DIR ;

AFFIN_EXEC=/home/pengine/prod/live_execs/cpu_affinity_mgr

## AFFINED_PID_FILE
AFFINED_PID_PROC=/spare/local/files/affinity_pid_process.txt ;

PIDFILE=$PIDDIR/$EXCH"_"$MODE"_"PIDfile.txt

# To run multiple fixfast instances for different exchanges -- 
LOGFILE="/spare/local/MDSlogs/"$EXCH$"_"$(date +"%Y%m%d")"_logfile.txt"

# For certification, we maintain a cleaner log.
CERTLOGFILE="/spare/local/MDSlogs/CERT_"$EXCH$"_"$(date +"%Y%m%d")"_logfile.txt"


## Required for MX 
MX_CRASH_INDICATOR_FILE=/spare/local/files/TMX/MXDataDaemonCrashed.dat;
MX_RS_MSG_SEQUENCE_START_FILE=/spare/local/files/TMX/tmx-rs-sequence.txt;
TODAY=`date +"%Y%m%d"` ;


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
    export VMA_LOG_FILE=/spare/local/logs/alllogs/FIXFAST_DEBUG.log
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="FIXFASTDAEMON"

fi


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/OAPI19.1/ ;


if [ $PWD != $HOME ] ; then cd $HOME ; fi

nw_file="$HOME/infracore_install/SysInfo/TradingInfo/NetworkInfo/network_account_info_filename.txt"
host=`hostname`
BCAST_IP=`grep $host $nw_file | grep "$EXCH" | awk '{print $4}'`
BCAST_PORT=`grep $host $nw_file | grep "$EXCH" | awk '{print $5}'`
if [[ "$BCAST_PORT" != +([0-9]) ]] ; then BCAST_PORT=12345 ; BCAST_IP="127.0.0.1" ; fi

if [ $CMD == "START" ]
then

    if [ $BCAST_IP == "127.0.0.1" ]
    then

        echo "N/w Info Invalid" ;
        echo "Data Daemon" $EXCH " " $MODE " " `hostname` " Got Loopback Address" | /bin/mail -s "Data Daemon Invalid IP/PORT" -r "networkrefinfo" "nseall@tworoads.co.in" ;
	exit ;

    fi

fi

case $EXCH in
    CME)
	;;
    EUREX)
	;;
    BMF)
	;;
    NTP)
	;;
    LIFFE)
	;;
    TMX)
	MDS_EXEC=/home/pengine/prod/live_execs/TMX-mds
	LOGFILE=/dev/stderr

        if [ -f $MX_CRASH_INDICATOR_FILE ]
        then

            LAST_CRASHED_DATE=`tail -1 $MX_CRASH_INDICATOR_FILE` ;

            if [ "$LAST_CRASHED_DATE" == "$TODAY" ]
            then

               #crashed today, update the saos and cxlord by an offset to make sure that orders go through

		touch $MX_RS_MSG_SEQUENCE_START_FILE ;

		if [ ! -f $MX_RS_MSG_SEQUENCE_START_FILE ]
		then 

	            echo "Can't Create RS seq file after MX Crash" | /bin/mail -s "MX Data Daemon RS Seqeunce Alert" -r "MXDataDaemon" "nseall@tworoads.co.in" ;
		    exit ;

		else 

	            echo "9999999999" > $MX_RS_MSG_SEQUENCE_START_FILE ;

		fi 

	    else 

		rm -rf $MX_CRASH_INDICATOR_FILE   # clear old file

            fi

        fi

	;;

    *)
	echo "Not implemented for $EXCH";
esac

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi

            if [ $ACR == "ON" ]
	    then 

	        onload $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT 2>$LOGFILE &

	    else 

	        if [ $ACR == "VMA" ]
		then 

	            LD_PRELOAD=libvma.so $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT 2>$LOGFILE &

		else

	            $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT 2>$LOGFILE &

		fi


	    fi 

	    MDSPID=$!
	    echo $MDSPID > $PIDFILE

            if [ $AFO == "AF" ]
            then

                echo $MDSPID $EXCH"DataLogger" >> $AFFINED_PID_PROC

                if [ -f $AFFIN_EXEC ] ;
                then
                    # Assign affinity to this exec.
                    $AFFIN_EXEC ASSIGN $MDSPID > /home/dvcinfra/mktDD.COUT.CERR ;
                fi

            fi

	fi    
	;;

    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
	    MDSPID=`tail -1 $PIDFILE`
	    proc_status_=`kill -2 $MDSPID 2>&1`;

            proc_status_kill_flag_=`echo $proc_status_ | grep "No such process" | wc -l` ;

            if [ $proc_status_kill_flag_ -gt 0 ] 
            then 

		touch $MX_CRASH_INDICATOR_FILE ;

		if [ ! -f $MX_CRASH_INDICATOR_FILE ]
		then

	            echo "Failed to Create MX Daemon crash indicator file" | /bin/mail -s "MX Data Daemon RS Seqeunce Alert" -r "MXDataDaemon" "nseall@tworoads.co.in" ; 


		else

		    echo `date +"%Y%m%d"` > $MX_CRASH_INDICATOR_FILE ;

		fi


            fi 

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
	    echo "Cannot stop an instance of $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT since $PIDFILE does not exist"

	fi    
	;;

    force_start|FORCE_START)
	if [ -f $PIDFILE ] ;
	then
	    MDSPID=`tail -1 $PIDFILE`;
	    running_proc_string=`ps -p $MDSPID -o comm=`;
	    if [ $running_proc_string ] ; then
   	        # print error & exit
		echo "Cannot start an instance of $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT since $PIDFILE exists and running_proc_string = $running_proc_string"
	    else
		if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
		if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi
		$MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT &
		MDSPID=$!
		echo $MDSPID > $PIDFILE
	    fi
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
	    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi
	    $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT &
	    MDSPID=$!
	    echo $MDSPID > $PIDFILE
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
	    MDSPIDLIST=`ps -efH | grep "$MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT" | grep -v grep | awk '{printf "%s ", $2}'`
	    kill -2 $MDSPIDLIST
	    sleep 1;
	    running_proc_string=`ps -p $MDSPID -o comm=`;
	    if [ $running_proc_string ] ; then
		kill -9 $MDSPIDLIST
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

