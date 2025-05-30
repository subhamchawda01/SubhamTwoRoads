#!/bin/bash
export ZF_ATTR=interface=enp1s0f1np1;

print_msg_and_exit () {
    echo $* ;
    exit ;
}

update_system_settings () {

    export ZF_ATTR=interface=enp1s0f0np0
    ulimit -c unlimited
    if [ $PWD != $HOME ] ; then cd $HOME ; fi

    TODAY=`date +"%Y%m%d"` ;
    TODAY_START_TIME=`date -d $TODAY +%s` ;
    CURRENT_TIME=`date +"%s"` ;

#TODO: replace hostname check with something more robust for CFE dst changes
#for CFE servers, explore timezone in last 4 hours of the day
    hostname_=`hostname | awk -F"-" '{print $2}'`;
    if [ "$hostname_" == "cfe" ]; then
        if [ $CURRENT_TIME -gt $((TODAY_START_TIME+72000)) ] && [ $CURRENT_TIME -lt $((TODAY_START_TIME+86400)) ] ; then
            export TZ="Asia/Tokyo";
        fi
    else
        if [ $CURRENT_TIME -gt $((TODAY_START_TIME+75600)) ] && [ $CURRENT_TIME -lt $((TODAY_START_TIME+86400)) ] ; then
            export TZ="Asia/Tokyo";
        fi
    fi
}

load_msg_accelerator () {

    NW_ACCELERATOR_FILE="/home/pengine/prod/live_configs/which_msg_accelerator_should_i_use_for_"`hostname` ;
    ACR=`cat $NW_ACCELERATOR_FILE` ;
    ACCELERATOR_PREFIX="" ;

    if [ "$ACR" == "ONLOAD" ] ; then

   #Default params
        export EF_LOG_VIA_IOCTL=1 ;
        export EF_NO_FAIL=0 ;
        export EF_SPIN_USEC=-1 ; export EF_POLL_USEC=-1 ; export EF_SELECT_SPIN=1 ;
        export EF_MULTICAST_LOOP_OFF=0 ;
        export EF_SHARE_WITH=-1;
        export EF_NAME=DEFAULT_STACK ;

        PARAM_OVERRIDE=$2;

        ONLOAD_PARAM_FILE="/home/pengine/prod/live_configs/which_onload_params_should_i_use_for_"`hostname` ;
   #Override params 
        if [ -f $ONLOAD_PARAM_FILE ] && [ -e $ONLOAD_PARAM_FILE ] && [ -s $ONLOAD_PARAM_FILE ] && [ -r $ONLOAD_PARAM_FILE ] && [ "$PARAM_OVERRIDE" == "OVERRIDE" ] ; then
            for paramvalue in `cat $ONLOAD_PARAM_FILE`; do
                export $paramvalue ;
            done
        fi

        STACK_INDICATION_FILE="/home/pengine/prod/live_configs/which_onload_stack_system_should_i_use_for_"`hostname` ;
        STACK_SYSTEM=`cat $STACK_INDICATION_FILE`;

        if [ "$STACK_SYSTEM" == "MULTI" ] ;
        then
            export EF_NAME=$1"_"STACK ;
        fi

        LOOPBACK_TRAFFIC_INDICATION_FILE="/home/pengine/prod/live_configs/which_loopback_acceleration_should_i_use_for_"`hostname` ;
        LOOPBACK_SYSTEM=`cat $LOOPBACK_TRAFFIC_INDICATION_FILE` ;

        if [ "$LOOPBACK_SYSTEM" == "ONLOAD" ] ; then

            if [ "$1" == "ORS" ] ; then
                export EF_FORCE_TCP_NODELAY=1;
                export EF_MCAST_SEND=3 ;
            else
                export EF_MCAST_RECV_HW_LOOP=1 ;
            fi

        fi

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

    elif [ "$ACR" == "EXASOCK" ] ; then
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/Exanic-1.8.1/ ;
        ACCELERATOR_PREFIX="exasock" ;

    else

        echo "do_nothing" >/dev/null ;

    fi

}


stop_strat_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $ORS_EXEC WITH CONFIG FILE $ORS_CONFIG_FILE SINCE $PIDFILE DOESN'T EXISTS" ;

    logout_and_stop ;

    STRATPID=`tail -1 $PIDFILE` ;
    kill -2 $STRATPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $STRATPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 5 ;
        running_proc_string=`ps -p $STRATPID -o comm=`;
        if [ $running_proc_string ] ; then
                echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $STRATPID" ;
                kill -9 $STRATPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;
}


logout_and_stop () {

    sleep 3 ; $ORS_COMMAND_EXEC $EXCH $PROFILE LOGOUT >> $OUTPUTLOGDIR/queryoutput/single_strat_engine.${PROGID}.COUT.CERR$TODAY 2>&1 &
    sleep 3 ; $ORS_COMMAND_EXEC $EXCH $PROFILE STOP >> $OUTPUTLOGDIR/queryoutput/single_strat_engine.${PROGID}.COUT.CERR$TODAY 2>&1 &

}


start_and_login () {
    ADD_TS_SCRIPT=/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh

        sleep 20 ; $ORS_COMMAND_EXEC $EXCH $PROFILE START >> $OUTPUTLOGDIR/queryoutput/single_strat_engine.${PROGID}.COUT.CERR$TODAY 2>&1
        sleep 50;
	#addts here
	#$ADD_TS_SCRIPT $ADD_TS_CONFIG >/dev/null 2>&1 ;

	$ADD_TS_SCRIPT /home/pengine/prod/live_configs/`hostname`_addts.cfg >/dev/null 2>&1 ;
}



start_strat_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $STRAT_EXEC WITH CONFIG FILE $ORS_CONFIG_FILE SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;
    [ ! -f $tradesfile ] || cp $tradesfile $tradesfile"_`date "+%s"`" ;
    [ ! -f $positionfile ] || cp $positionfile $positionfile"_`date "+%s"`" ;
    
    $STRAT_EXEC $CONFIGFILE $LIVE_FILE $YYYYMMDD $start_time $end_time /spare/local/ORSlogs/${TYPE}/${PROFILE} $PROGID >> $OUTPUTLOGDIR/queryoutput/single_strat_engine.${PROGID}.COUT.CERR$TODAY 2>&1 &
    STRATPID=$! ;
    echo $STRATPID > $PIDFILE ;

    start_and_login ;
    cd /spare/local/ORSlogs/${TYPE}/${PROFILE}
    chgrp infra log.$TODAY
    chgrp infra audit*$TODAY.in
    chgrp infra audit*$TODAY.out
    exit ;

}

####Initialize params and arguments required for ORS
init_single_strat () {

    [ $# -eq 7 ] || print_msg_and_exit "Usage : < script > < PROFILE > < LIVE_FILE >  START_TIME END_TIME PROGRAM_ID NSE_EQ/NSE_FO START/STOP >" ;

    STRAT_EXEC="/home/dvctrader/ATHENA/single_process_trade_engine_live_t2t_read_trigger_20210811_bug_fix_secnotfound" ;
    ORS_COMMAND_EXEC="/home/pengine/prod/live_scripts/ors_control.pl" ;

    [ -f $STRAT_EXEC ] || print_msg_and_exit "STRAT EXEC -> $STRAT_EXEC DOESN'T EXIST" ;
    [ -f $ORS_COMMAND_EXEC ] || print_msg_and_exit "ORS CONTROL EXEC -> $ORS_COMMAND_EXEC DOESN'T EXIST" ;

    PROFILE=$1 ;
    LIVE_FILE=$2;
    start_time=$3;
    end_time=$4;
    PROGID=$5;
    TYPE=$6
    CMD=$7 ;

    CONFIGFILE="/home/pengine/prod/live_configs/common_"$PROFILE"_ors.cfg" ;
# /home/pengine/prod/live_configs/common_MSEQ7_ors.cfg

  # Test if file is good, readable and non-zero size 
    [ -f $CONFIGFILE -a -s $CONFIGFILE -a -r $CONFIGFILE ] || print_msg_and_exit "STRAT_CONFIG_FILE -> $CONFIGFILE EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    EXCH=`grep "Exchange " $CONFIGFILE | awk '{print $2}'` ;

    [ ! -z $EXCH ] || print_msg_and_exit "ORS_CONFIG_FILE -> $CONFIGFILE DOESN'T HAVE AN EXCHANGE SPECIFIED" ;

    PIDFILE=/spare/local/logs/tradelogs/PID_STRAT_DIR/$EXCH"_"$PROGID"_"PIDfile.txt ;
    OUTPUTLOGDIR=/spare/local/logs/tradelogs/;
 
    logfile=$OUTPUTLOGDIR"log.`date "+%Y%m%d"`"
    positionfile=$OUTPUTLOGDIR"position.`date "+%Y%m%d"`"
    tradesfile=$OUTPUTLOGDIR"trades.`date "+%Y%m%d"`"

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/ATHENA/
    update_system_settings ;
    load_msg_accelerator "ORS" ;
    [ ! "$CMD" == "START" ] || start_strat_process ;
    [ ! "$CMD" == "STOP" ] || stop_strat_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}



YYYYMMDD=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi


init_single_strat $*
