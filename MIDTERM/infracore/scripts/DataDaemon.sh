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

    if [ "$ACR" == "ONLOAD" ] || [ "$ACR" == "ON" ] ; then

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

####Initialize params and arguments required for mktDD

init () {

    [ $# -eq 3 ] || print_msg_and_exit "Usage : < script > < CMD > < EXCH > < MODE - REFERENCE/DATA/LOGGER/NCLOGGER > < OPT BCAST_IP > < OPT BCAST_PORT > < OPT LOG_DIR >" ;

    MKTDD_EXEC="/home/pengine/prod/live_execs/mktDD" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $MKTDD_EXEC ] || print_msg_and_exit "EXEC -> $MKTDD_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    EXCH=$2 ; 
    MODE=$3 ; 

  # Test if file is good, readable and non-zero size 
    [ -f $MKTDD_EXEC -a -s $MKTDD_EXEC -a -r $MKTDD_EXEC ] || print_msg_and_exit "MKTDD EXEC  -> $MKTDD_EXEC EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;
    [ -f $AFFIN_EXEC -a -s $AFFIN_EXEC -a -r $AFFIN_EXEC ] || print_msg_and_exit "MKTDD EXEC  -> $AFFIN_EXEC EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"MKTDD_"$EXCH"_"$MODE"_PIDfile.txt" ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ;

    update_system_settings ;
    load_msg_accelerator ;

    logfile=$OUTPUTLOGDIR"mktdd_"$EXCH"_"$MODE"."`date "+%Y%m%d"`".coutcerr" ; 

    [ ! "$CMD" == "START" ] || start_process ;
    [ ! "$CMD" == "STOP" ] || stop_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $MKTDD_EXEC FOR $EXCH WITH MODE $MODE SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    $ACCELERATOR_PREFIX $MKTDD_EXEC --exchange $EXCH --mode $MODE >>$logfile 2>&1 &
    MKTDDPID=$! ;
    echo $MKTDDPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $MKTDDPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    EXEC_NAME=`echo "MKTDD-"$EXCH"-"$MODE` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $MKTDDPID $EXEC_NAME >>$logfile 2>&1

    exit ;

}

stop_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $MKTDD_EXEC FOR $EXCH WITH MODE $MODE SINCE $PIDFILE DOESN'T EXISTS" ;

    MKTDDPID=`tail -1 $PIDFILE` ;
    kill -2 $MKTDDPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $MKTDDPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $MKTDDPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $MKTDDPID" ;
            kill -9 $MKTDDPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init $*
