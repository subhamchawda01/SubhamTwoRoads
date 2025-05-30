#!/bin/bash

USAGE="$0 EXCH MODE CMD ACCELERATOR[ON/VMA/OFF] AFFINITY[AF/NF]";

if [ $# -lt 5 ] ; 
then 
    echo $USAGE;
    exit;
fi

MDS_EXEC=$HOME/LiveExec/bin/mktDD
AFFIN_EXEC=$HOME/LiveExec/bin/cpu_affinity_mgr

EXCH=$1; shift;
MODE=$1; shift; #DATA REFERENCE LOGGER
CMD=$1; shift;
ACR=$1; shift; #onload switch option
AFO=$1; shift;
if [ $# == 1 ] ; then  OPTARG=$1 ; else OPTARG="DUMMY" ; fi ; #optional arguments OPTARG=$1; shift; #optional arguments

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


OUTPUTLOGDIR=/spare/local/MDSlogs/$EXCH ;
PIDDIR=/spare/local/MDSlogs/PID_MDS_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$MODE"_"PIDfile.txt
if [ $OPTARG == "ALPES" ] ; then PIDFILE=$PIDDIR/$EXCH"_"$MODE"_ALPES_PIDfile.txt" ; fi

YYYYMMDD=$(date "+%Y%m%d");

# To run multiple fixfast instances for different exchanges -- 
LOGFILE="/spare/local/MDSlogs/"$EXCH$"_"$YYYYMMDD"_"$MODE"_logfile.txt";
if [ $OPTARG == "ALPES" ] ; then LOGFILE="/spare/local/MDSlogs/"$EXCH$"_"$YYYYMMDD"_"$MODE"_ALPES_logfile.txt" ; fi

# For certification, we maintain a cleaner log.
CERTLOGFILE="/spare/local/MDSlogs/CERT_"$EXCH$"_"$YYYYMMDD"_logfile.txt";

# COUT - CERR output
COUTCERRFILE=/home/dvcinfra/mktDD.COUT.CERR;
COUTCERRFILE="/spare/local/MDSlogs/"$EXCH$"_"$YYYYMMDD"_"$MODE"_cout_cerr_file.txt";

if [ $PWD != $HOME ] ; then cd $HOME ; fi

if [ $MODE == "DATA" ]
then
    EXCH_TMP=$EXCH

    nw_file="$HOME/infracore_install/SysInfo/TradingInfo/NetworkInfo/network_account_info_filename.txt"
    host=`hostname`
    if [ $OPTARG == "ALPES" ] ; then host="ALPES" ; fi
    BCAST_IP=`grep $host $nw_file | grep -w "$EXCH_TMP" | awk '{print $4}'`
    BCAST_PORT=`grep $host $nw_file | grep -w "$EXCH_TMP" | awk '{print $5}'`
    if [[ "$BCAST_PORT" =~ [0-9]+ ]] ; then echo ''; else BCAST_PORT=12345 ; BCAST_IP="127.0.0.1" ; fi

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
    RTS)
	;;
    MICEX)
	;;
    MICEX_EQ)
	;;
    MICEX_CR)
	;;
    BMF)
	;;
    NTP)
	;;
    LIFFE)
	;;
    CHIX)
	MDS_EXEC=$HOME/LiveExec/bin/chiXDD
	;;
    RTS_P2)
	MDS_EXEC=$HOME/LiveExec/bin/plaza2DD
	;;
    TMX)
	MDS_EXEC=$HOME/LiveExec/bin/TMX-mds
	LOGFILE=/dev/stderr
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

            # if the logfile already exists take a backup of the error_stream_log
            # used time with the filename to allow multiple backups
            if [ -f $LOGFILE ] ;
            then
                cp $LOGFILE $LOGFILE"_`date "+%s"`"
            fi

            case $MODE in
              # logger and reference mode don't require ip/port
		LOGGER )      

		    if [ $ACR == "ON" ] 
		    then 

	                 onload $MDS_EXEC --mode $MODE --exchange $EXCH 2>$LOGFILE >> $COUTCERRFILE &

		    else

		         if [ $ACR == "VMA" ] 
			 then

			     LD_PRELOAD=libvma.so $MDS_EXEC --mode $MODE --exchange $EXCH 2>$LOGFILE >> $COUTCERRFILE &

			 else

                             $MDS_EXEC --mode $MODE --exchange $EXCH 2>$LOGFILE >> $COUTCERRFILE &

			fi

		    fi

                    ;;
		REFERENCE )
		    if [ $ACR == "ON" ]
		    then
		     
	                 onload $MDS_EXEC --mode $MODE --exchange $EXCH 2>$LOGFILE &

		    else 

		         if [ $ACR == "VMA" ] 
			 then

			     LD_PRELOAD=libvma.so $MDS_EXEC --mode $MODE --exchange $EXCH 2>$LOGFILE &

			 else

		             $MDS_EXEC --mode $MODE --exchange $EXCH 2>$LOGFILE & 

			 fi

		    fi

                    ;;
		* )

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
            esac         

	    MDSPID=$!
	    echo $MDSPID > $PIDFILE

	    if [ $AFO == "AF" ] 
	    then

	        if [ -f $AFFIN_EXEC ] ;
	        then
	            # Assign affinity to this exec.
		    $AFFIN_EXEC ASSIGN $MDSPID "mktDD_"$EXCH"_"$MODE >> $COUTCERRFILE ;
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

