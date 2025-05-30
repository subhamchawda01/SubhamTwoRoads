#!/bin/bash

print_msg_and_exit () {

    echo $* ;
    exit ;

}

update_system_settings () {

    ulimit -c unlimited
    if [ $PWD != $HOME ] ; then cd $HOME ; fi

    TODAY=`date +"%Y%m%d"` ;
    TODAY_START_TIME=`date -d $TODAY +%s` ;
    CURRENT_TIME=`date +"%s"` ;

    if [ $CURRENT_TIME -gt $((TODAY_START_TIME+75600)) ] && [ $CURRENT_TIME -lt $((TODAY_START_TIME+86400)) ] ; then
	export TZ="Asia/Tokyo";
    fi

}

load_msg_accelerator () {

    NW_ACCELERATOR_FILE="/home/pengine/prod/live_configs/which_msg_accelerator_should_i_use_for_"`hostname` ;
    ACR=`cat $NW_ACCELERATOR_FILE` ;
    ACCELERATOR_PREFIX="" ;

    if [ "$ACR" == "ONLOAD" ] ; then

	export EF_LOG_VIA_IOCTL=1 ;
	export EF_NO_FAIL=0 ;
	export EF_UDP_RECV_MCAST_UL_ONLY=1 ;
	export EF_SPIN_USEC=-1 ; export EF_POLL_USEC=-1 ; export EF_SELECT_SPIN=1 ;
	export EF_MULTICAST_LOOP_OFF=0 ;
	export EF_MAX_ENDPOINTS=1024 ;
	export EF_SHARE_WITH=-1;
	export EF_NAME=ORS_STACK ;
	ACCELERATOR_PREFIX="onload" ;

    elif [ "$ACR" == "VMA" ] ; then

        export VMA_CONFIG_FILE=/spare/local/files/VMA/libvma.conf
	export VMA_MEM_ALLOC_TYPE=1; export VMA_FORK=1;
	export VMA_TRACELEVEL=3;
	export VMA_LOG_DETAILS=3;
	export VMA_LOG_FILE=/spare/local/logs/alllogs/ORS_DEBUG.log
	export VMA_QP_LOGIC=0;
	export VMA_RX_POLL=-1;
	export VMA_SELECT_POLL=-1;
	export VMA_APPLICATION_ID="ORS";
	export LD_PRELOAD=libvma.so ;

    else

	echo "do_nothing" >/dev/null ;

    fi

}

####Initialize params and arguments required for ORS

init () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ; 

    CW_EXEC="/home/pengine/prod/live_execs/CombinedShmWriter" ; 
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $CW_EXEC ] || print_msg_and_exit "EXEC -> $CW_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ; 
    COMBINED_WRITER_CFG="/home/pengine/prod/live_configs/"`hostname`"_combinedwriter.cfg" ; 

  # Test if file is good, readable and non-zero size 
    [ -f $COMBINED_WRITER_CFG -a -s $COMBINED_WRITER_CFG -a -r $COMBINED_WRITER_CFG ] || print_msg_and_exit "CW_CONFIG_FILE -> $COMBINED_WRITER_CFG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"CombinedShmWriter_"PIDfile.txt ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ; 

    update_system_settings ;
    load_msg_accelerator ;

    logfile=$OUTPUTLOGDIR"combined_writer_log.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_process ;
    [ ! "$CMD" == "STOP" ] || stop_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $CW_EXEC WITH CONFIG FILE $COMBINED_WRITER_CFG SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;


    $ACCELERATOR_PREFIX $CW_EXEC --config $COMBINED_WRITER_CFG >>$logfile 2>&1 & 
    CWPID=$! ;
    echo $CWPID > $PIDFILE ; 

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ; 
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $CWPID ; 
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ; 

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ; 
    AF_FLAG=`cat $AF_FLAG_FILE` ; 

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $CWPID "CombinedShmWriter" >>$logfile 2>&1 

    exit ; 

}

stop_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $CW_EXEC WITH CONFIG FILE $COMBINED_WRITER_CFG SINCE $PIDFILE DOESN'T EXISTS" ;

    CWPID=`tail -1 $PIDFILE` ;
    kill -2 $CWPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $CWPID -o comm=`;

    if [ $running_proc_string ] ; then
	sleep 15 ;
	running_proc_string=`ps -p $CWPID -o comm=`;
	if [ $running_proc_string ] ; then
	    echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $CWPID" ;
	    kill -9 $CWPID >/dev/null 2>/dev/null ;
	fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init $*
