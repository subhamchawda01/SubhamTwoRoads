#!/bin/bash

USAGE="$0 EXCH MODE CMD ACCELERATOR[ON/VMA/OFF] AFFINITY[AF/NF]";

if [ $# -lt 5 ] ; 
then 
    echo $USAGE;
    exit;
fi

MDS_EXEC=/home/pengine/prod/live_execs/integDD
AFFIN_EXEC=/home/pengine/prod/live_execs/cpu_affinity_mgr

EXCH=INTEGRATED
MODE=$1; shift; #DATA REFERENCE LOGGER
CMD=$1; shift;
ACR=$1; shift; #onload switch option
AFO=$1; shift;

if [ $ACR == "ON" ]
then

   export EF_LOG_VIA_IOCTL=1 ;
   export EF_NO_FAIL=0 ;
   export EF_SPIN_USEC=-1 ; export EF_POLL_USEC=-1 ; export EF_SELECT_SPIN=1 ;
   export EF_MULTICAST_LOOP_OFF=0 ;
   export EF_MAX_ENDPOINTS=1024 ;
   export EF_SHARE_WITH=-1;
   export EF_NAME=DEFAULT_STACK;

fi

if [ $ACR == "VMA" ]
then

    export VMA_MEM_ALLOC_TYPE=1; export VMA_FORK=1;
    export VMA_TRACELEVEL=3;
    export VMA_LOG_DETAILS=3;
    export VMA_LOG_FILE=/spare/local/logs/alllogs/NEWFIXFAST_DEBUG.log
    export VMA_CONFIG_FILE=/spare/local/files/VMA/libvma.conf 
    export VMA_QP_LOGIC=0;
    export VMA_RX_POLL=-1;
    export VMA_SELECT_POLL=-1;
    export VMA_APPLICATION_ID="MKTDD";

fi

#export NEW_GCC_LIB=/usr/local/lib
#export NEW_GCC_LIB64=/usr/local/lib64
#export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH
#
#export GCC_4_8_ROOT=/apps/gcc_versions/gcc-4_8_install
#if [ -d $GCC_4_8_ROOT ] ; then
#   export PATH=$GCC_4_8_ROOT/bin:$PATH;
#   export LD_LIBRARY_PATH=$GCC_4_8_ROOT/lib64:$LD_LIBRARY_PATH ;
#fi

DATA_DIR=/spare/local/MDSlogs/NonCombined/

#cleaup old data 
find $DATA_DIR -type f -mtime +3 -exec rm {} \;

## AFFINED_PID_FILE
AFFINED_PID_PROC=/spare/local/files/affinity_pid_process.txt ;

PIDDIR=/spare/local/MDSlogs/PID_MDS_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$MODE"_NonCombinedLogger_"PIDfile.txt

YYYYMMDD=$(date "+%Y%m%d");

# To run multiple fixfast instances for different exchanges -- 
LOGFILE="/spare/local/MDSlogs/"$EXCH$"_"$YYYYMMDD"_"$MODE"_logfile.txt";

# For certification, we maintain a cleaner log.
CERTLOGFILE="/spare/local/MDSlogs/CERT_"$EXCH$"_"$YYYYMMDD"_logfile.txt";

# COUT - CERR output
COUTCERRFILE=/home/dvcinfra/mktDD.COUT.CERR;
COUTCERRFILE="/spare/local/MDSlogs/"$EXCH$"_"$YYYYMMDD"_"$MODE"_cout_cerr_file.txt";

if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $MDS_EXEC --mode $MODE since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi

            # if the logfile already exists take a backup of the error_stream_log
            # used time with the filename to allow multiple backups
            if [ -f $LOGFILE ] ;
            then
                cp $LOGFILE $LOGFILE"_`date "+%s"`"
            fi

            case $MODE in
		INTEGRATED )
                    config_string=""
                    #Generate config string
                    for arg_ in $*;
                    do
                       config_string=$config_string" --exchange-mode $arg_ "
                    done
		    if [ $ACR == "ON" ]
		    then
		     
	                 onload $MDS_EXEC --mode $MODE $config_string --log_dir $DATA_DIR >$LOGFILE 2>&1 &

		    else 

		         if [ $ACR == "VMA" ] 
			 then

			     LD_PRELOAD=libvma.so $MDS_EXEC --mode $MODE $config_string --log_dir $DATA_DIR >$LOGFILE 2>&1 &

			 else

		             $MDS_EXEC --mode $MODE $config_string --log_dir $DATA_DIR >$LOGFILE 2>&1 &

			 fi

		    fi

                    ;;
		* )
                    echo "Mode not implemented"
                    exit 1
            esac         

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
	    echo "Cannot stop an instance of $MDS_EXEC --mode $MODE --exchange $EXCH --bcast_ip $BCAST_IP --bcast_port $BCAST_PORT since $PIDFILE does not exist"
	fi    
	;;

    *)
	echo CMD $CMD not expected
	;;
esac

