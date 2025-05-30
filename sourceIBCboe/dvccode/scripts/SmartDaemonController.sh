#!/bin/bash

#=======================================================================  GENERIC FUNCTIONS 

print_msg_and_exit () {
    echo $* ;
    exit ;
}

update_system_settings () {

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/OAPI.19.3/:/spare/local/lib/OAPIGenium/:/spare/local/lib/SGXOAPI_OUCH_4.1_1307/;
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

    # ONLOAD params for OSE (temporary unitl we move to multi-stack)
    hostname_=`hostname | awk -F"-" '{print $2}'`;
    if [ "$hostname_" == "ose" ] || [ "$hostname_" == "OSE" ]; then
        export EF_UDP_RCVBUF=90000
        export EF_MAX_PACKETS=90000
        export EF_RXQ_SIZE=4096
    fi

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

#======================================================================= SPECIFIC FUNCTIONS 

load_exchange_specific_settings () {

    case $1 in

	LIFFE|liffe )

            LIFFE_LOGON_SEQ_FILE=$OUTPUTLOGDIR/CCG.1.7.logong.last.seqnum.log

            if [ -f $LIFFE_LOGON_SEQ_FILE ]
            then
		>$LIFFE_LOGON_SEQ_FILE;
		echo "-1" > $LIFFE_LOGON_SEQ_FILE ;
            else
		>$LIFFE_LOGON_SEQ_FILE ;
		echo "0" > $LIFFE_LOGON_SEQ_FILE ;
            fi
	    ;;

	*)

	    ;;

    esac

}

start_and_login () {

    ADD_TS_SCRIPT=/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh

    if [ "$EXCH" == "SGX" ] ; then
	$ADD_TS_SCRIPT $ADD_TS_CONFIG >/dev/null 2>&1 ;
    fi

    if [ "$EXCH" == "LIFFE" ] ; then

	sleep 5 ; $ORS_COMMAND_EXEC $EXCH $PROFILE START >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
	sleep 1 ; $ORS_COMMAND_EXEC $EXCH $PROFILE LOGIN >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;

    else

	sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE START >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
	sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE LOGIN >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;

    fi
    
    if [ "$EXCH" != "SGX" ] ; then
	$ADD_TS_SCRIPT $ADD_TS_CONFIG >/dev/null 2>&1 ;
    fi

}

logout_and_stop () {

    sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE LOGOUT >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
    sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE STOP >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;

}

start_ors_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $ORS_EXEC WITH CONFIG FILE $ORS_CONFIG_FILE SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;
    [ ! -f $tradesfile ] || cp $tradesfile $tradesfile"_`date "+%s"`" ;
    [ ! -f $positionfile ] || cp $positionfile $positionfile"_`date "+%s"`" ;

    [ "$POS" == "KEEP" ] || rm $positionfile ;

    $ACCELERATOR_PREFIX $ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 &
    ORSPID=$! ;
    echo $ORSPID > $PIDFILE ;

    start_and_login ;

    exit ;

}

stop_ors_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $ORS_EXEC WITH CONFIG FILE $ORS_CONFIG_FILE SINCE $PIDFILE DOESN'T EXISTS" ;

    logout_and_stop ;

    ORSPID=`tail -1 $PIDFILE` ;
    kill -2 $ORSPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $ORSPID -o comm=`;

    if [ $running_proc_string ] ; then
	sleep 15 ;
	running_proc_string=`ps -p $ORSPID -o comm=`;
	if [ $running_proc_string ] ; then
	    if [ "$EXCH" == "SGX" ] ; then
		echo "Please try stopping SGX ORS Again with SIGINT if logout pending. Otherwise kill with SIGKILL." | /bin/mail -s "SGX ORS Failed to Stop on SIGINT" -r "SmartDaemonController" "pranjal.jain@tworoads.co.in, vedant@tworoads.co.in, ravi.parikh@tworoads.co.in";
	    else
		echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $ORSPID" ;
		kill -9 $ORSPID >/dev/null 2>/dev/null ;
	    fi
	fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

force_restart_ors () {
  kill -9 $ORSPID >/dev/null 2>/dev/null ; 
  rm -rf $PIDFILE ; 
  start_ors_process ;
}

start_and_login_dc () {

    if [ "$EXCH" == "LIFFE" ] ; then
	sleep 1 ; $DC_ORS_COMMAND_EXEC $EXCH $PROFILE START >> $OUTPUTLOGDIR/dc_cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
	sleep 2 ; $DC_ORS_COMMAND_EXEC $EXCH $PROFILE LOGIN >> $OUTPUTLOGDIR/dc_cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
    else
	sleep 10 ; $DC_ORS_COMMAND_EXEC $EXCH $PROFILE START >> $OUTPUTLOGDIR/dc_cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
	sleep 10 ; $DC_ORS_COMMAND_EXEC $EXCH $PROFILE LOGIN >> $OUTPUTLOGDIR/dc_cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
    fi
}

logout_and_stop_dc () {

    sleep 10 ; $DC_ORS_COMMAND_EXEC $EXCH $PROFILE LOGOUT >> $OUTPUTLOGDIR/dc_cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
    sleep 10 ; $DC_ORS_COMMAND_EXEC $EXCH $PROFILE STOP >> $OUTPUTLOGDIR/dc_cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;

}

start_dc_ors_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $DC_ORS_EXEC WITH CONFIG FILE $DC_ORS_CONFIG_FILE SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;
    [ ! -f $tradesfile ] || cp $tradesfile $tradesfile"_`date "+%s"`" ;

    $DC_ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR >> $OUTPUTLOGDIR/dc_cme_ilink_ors.COUT.CERR$TODAY 2>&1 &
    DC_ORSPID=$! ;
    echo $DC_ORSPID > $PIDFILE ;

    start_and_login_dc ;

    exit ;

}

stop_dc_ors_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $DC_ORS_EXEC WITH CONFIG FILE $DC_ORS_CONFIG_FILE SINCE $PIDFILE DOESN'T EXISTS" ;

    logout_and_stop_dc ;

    DC_ORSPID=`tail -1 $PIDFILE` ;
    kill -2 $DC_ORSPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $DC_ORSPID -o comm=`;

    if [ $running_proc_string ] ; then
	sleep 15 ;
	running_proc_string=`ps -p $DC_ORSPID -o comm=`;
	if [ $running_proc_string ] ; then
	    echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $DC_ORSPID" ;
	    kill -9 $DC_ORSPID >/dev/null 2>/dev/null ;
	fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

start_sgxdatadaemon_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $MKTDD_EXEC FOR $EXCH SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    $ACCELERATOR_PREFIX $MKTDD_EXEC --config $SGX_MKTDD_CFG >>$logfile 2>&1 &

    MKTDDPID=$! ;
    echo $MKTDDPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $MKTDDPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    EXEC_NAME=`echo "MKTDD-"$EXCH` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $MKTDDPID $EXEC_NAME >>$logfile 2>&1

    exit ;

}

stop_sgxdatadaemon_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $MKTDD_EXEC FOR $EXCH SINCE $PIDFILE DOESN'T EXISTS" ;

    MKTDDPID=`tail -1 $PIDFILE` ;
    kill -2 $MKTDDPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $MKTDDPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $MKTDDPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "Please try stopping SGX Mkt Data Again with SIGINT. Don't kill SGX Mkt Data via SIGKILL." | /bin/mail -s "SGX Mkt Data Failed to Stop on SIGINT" -r "SmartDaemonController" "pranjal.jain@tworoads.co.in , nseall@tworoads.co.in"; 
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

start_datadaemon_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $MKTDD_EXEC FOR $EXCH WITH MODE $MODE SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    if [ "DATA" == "$3" ] && [ $# -eq 5 ] ; then 
	$ACCELERATOR_PREFIX $MKTDD_EXEC --exchange $EXCH --mode $MODE --bcast_ip $4 --bcast_port $5 >>$logfile 2>&1 &
    elif [ "LOGGER" == "$4" ] && [ $# -eq 4 ] ; then 
	$ACCELERATOR_PREFIX $MKTDD_EXEC --exchange $EXCH --mode $MODE --log_dir $4 >>$logfile 2>&1 &
    else 
	$ACCELERATOR_PREFIX $MKTDD_EXEC --exchange $EXCH --mode $MODE >>$logfile 2>&1 &
    fi 

    MKTDDPID=$! ;
    echo $MKTDDPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $MKTDDPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;

}

stop_datadaemon_process () {

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

start_realshmpacketslogger_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $REALSHMPACKETS_LOGGER_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    $ACCELERATOR_PREFIX $REALSHMPACKETS_LOGGER_EXEC  >>$logfile 2>&1 &

    REALSHMPACKETSLOGGERPID=$! ;
    echo $REALSHMPACKETSLOGGERPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $REALSHMPACKETSLOGGERPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $REALSHMPACKETSLOGGERPID "RealShmPacketsLogger"  >>$logfile 2>&1

    exit ;

}

stop_realshmpacketslogger_process () {
    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $REALSHMPACKETS_LOGGER_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;

    REALSHMPACKETSLOGGERPID=`tail -1 $PIDFILE` ;
    kill -2 $REALSHMPACKETSLOGGERPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $REALSHMPACKETSLOGGERPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $REALSHMPACKETSLOGGERPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $REALSHMPACKETSLOGGERPID" ;
            kill -9 $REALSHMPACKETSLOGGERPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}


start_combinedsource_process () {
	#start of chmWriter denotes start of trading day(restart does not)
	#if this a first start, touch a file (/tmp/.start_of_day_YYYYMMDD)
    tmp_start_file="/tmp/.start_of_day_`date +\%Y\%m\%d`";
    if [ ! -f $tmp_start_file ]; then
	rm -f /tmp/.start_of_day_*; #delete all files for previous days
	touch $tmp_start_file;
	start_of_day_script=/home/pengine/prod/live_scripts/process_start_of_day.sh
	if [ -f $start_of_day_script ]; then
			#now start a script in background that processes everything that needs to be done at start of day
	    $start_of_day_script >/dev/null 2>&1 &
	fi 
    fi
    
    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $CW_EXEC WITH CONFIG FILE $COMBINED_WRITER_CFG SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;


    $ACCELERATOR_PREFIX $CW_EXEC --config $COMBINED_WRITER_CFG >>$logfile 2>&1 &
    CWPID=$! ;
    echo $CWPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $CWPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;

}

stop_combinedsource_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $CW_EXEC WITH CONFIG FILE $COMBINED_WRITER_CFG SINCE $PIDFILE DOESN'T EXISTS" ;

    CWPID=`tail -1 $PIDFILE` ;
    kill -2 $CWPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $CWPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $CWPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $CWPID" ;
            sleep 60 ;
            if [ `ps -ef |grep CombinedShmWriter | grep "$CWPID" | grep -v "grep" | wc -l` -gt 0 ] ; then 
		kill -9 $CWPID >/dev/null 2>/dev/null ;
            fi
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

start_combinedmulticaster_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $CM_EXEC FOR EXCHANGE $EXCH SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;


#    $ACCELERATOR_PREFIX $CM_EXEC --exchange $EXCH >>$logfile 2>&1 &
#Since we know this consumer process works off shm and has no need of msg accelerator
    if [ $EXCH = "CONFIG" ]; then
	COMBINED_MULTICASTER_CFG="/home/pengine/prod/live_configs/"`hostname`"_combinedmulticaster.cfg" ;
      # Test if file is good, readable and non-zero size 
	[ -f $COMBINED_MULTICASTER_CFG -a -s $COMBINED_MULTICASTER_CFG -a -r $COMBINED_MULTICASTER_CFG ] || print_msg_and_exit "CM_CONFIG_FILE -> $COMBINED_MULTICASTER_CFG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;
	$ACCELERATOR_PREFIX $CM_EXEC --config $COMBINED_MULTICASTER_CFG >>$logfile 2>&1 &
    else
	if [ -z "$PRODUCTS_FILTER" ]; then
	    $ACCELERATOR_PREFIX $CM_EXEC --exchange $EXCH >>$logfile 2>&1 &
	else
	    $ACCELERATOR_PREFIX $CM_EXEC --exchange $EXCH --products_filter $PRODUCTS_FILTER >>$logfile 2>&1 &
	fi 
    fi
    
    CMPID=$! ;
    echo $CMPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $CMPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;

}

stop_combinedmulticaster_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $CM_EXEC FOR EXCHANGE $EXCH SINCE $PIDFILE DOESN'T EXISTS" ;

    CMPID=`tail -1 $PIDFILE` ;
    kill -2 $CMPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $CMPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $CMPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $CMPID" ;
            kill -9 $CMPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}


start_l1_sender_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $CM_EXEC FOR EXCHANGE $EXCH SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;


    $ACCELERATOR_PREFIX $CM_EXEC $EXCH $CONFIG_FILE >>$logfile 2>&1 &
    
    CMPID=$! ;
    echo $CMPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $CMPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $CMPID "l1_sender-"`echo $EXCH` >>$logfile 2>&1

    exit ;
}

stop_l1_sender_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $CM_EXEC FOR EXCHANGE $EXCH SINCE $PIDFILE DOESN'T EXISTS" ;

    CMPID=`tail -1 $PIDFILE` ;
    kill -2 $CMPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $CMPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $CMPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $CMPID" ;
            kill -9 $CMPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

####Initialize params and arguments required for ORS
init_ors () {

    [ $# -eq 3 ] || print_msg_and_exit "Usage : < script > < PROFILE > < CMD > < POSFILE ( KEEP / CLEAR ) >" ;

    ORS_EXEC="/home/pengine/prod/live_execs/cme_ilink_ors" ;
    ORS_COMMAND_EXEC="/home/pengine/prod/live_scripts/ors_control.pl" ;

    [ -f $ORS_EXEC ] || print_msg_and_exit "ORS EXEC -> $ORS_EXEC DOESN'T EXIST" ;
    [ -f $ORS_COMMAND_EXEC ] || print_msg_and_exit "ORS CONTROL EXEC -> $ORS_COMMAND_EXEC DOESN'T EXIST" ;

    PROFILE=$1 ;
    CMD=$2 ; 
    POS=$3 ;

    CONFIGFILE="/home/pengine/prod/live_configs/common_"$PROFILE"_ors.cfg" ;

  # Test if file is good, readable and non-zero size 
    [ -f $CONFIGFILE -a -s $CONFIGFILE -a -r $CONFIGFILE ] || print_msg_and_exit "ORS_CONFIG_FILE -> $CONFIGFILE EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    EXCH=`grep "Exchange " $CONFIGFILE | awk '{print $2}'` ;

    [ ! -z $EXCH ] || print_msg_and_exit "ORS_CONFIG_FILE -> $CONFIGFILE DOESN'T HAVE AN EXCHANGE SPECIFIED" ;

    PIDFILE=/spare/local/ORSlogs/PID_ORS_DIR/$EXCH"_"$PROFILE"_"PIDfile.txt ;
    OUTPUTLOGDIR=/spare/local/ORSlogs/$EXCH/$PROFILE/ ;

    logfile=$OUTPUTLOGDIR"log.`date "+%Y%m%d"`"
    positionfile=$OUTPUTLOGDIR"position.`date "+%Y%m%d"`"
    tradesfile=$OUTPUTLOGDIR"trades.`date "+%Y%m%d"`"

    load_exchange_specific_settings $EXCH ;
    update_system_settings ;
    load_msg_accelerator "ORS" ;

    [ ! "$CMD" == "START" ] || start_ors_process ; 
    [ ! "$CMD" == "STOP" ] || stop_ors_process ;
    [ ! "$CMD" == "FSTART" ] || force_restart_ors ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

init_dc_ors () {

    [ $# -eq 3 ] || print_msg_and_exit "Usage : < script > < EXCH > < PROFILE > < CMD >" ;

    DC_ORS_EXEC="/home/pengine/prod/live_execs/cme_ilink_ors" ;
    DC_ORS_COMMAND_EXEC="/home/pengine/prod/live_scripts/ors_control.pl" ;

    [ -f $ORS_COMMAND_EXEC ] || print_msg_and_exit "ORS CONTROL EXEC -> $ORS_COMMAND_EXEC DOESN'T EXIST" ;

    EXCH=$1 ;
    PROFILE=$2 ;
    CMD=$3 ; 

    if [ "$EXCH" == "LIFFE" ] ; then 
	DC_ORS_EXEC="/home/pengine/prod/live_execs/liffe_dropcopy_ors" ;
    fi

    if [ "$EXCH" == "CFE" ] ; then 
	DC_ORS_EXEC="/home/pengine/prod/live_execs/cfe_dropcopy";
    fi

    if [ "$EXCH" == "BMFEP" ] ; then
	DC_ORS_EXEC="/home/pengine/prod/live_execs/cme_ilink_ors_dropcopy";
    fi

    [ -f $DC_ORS_EXEC ] || print_msg_and_exit "DC_ORS EXEC -> $DC_ORS_EXEC DOESN'T EXIST" ;

    CONFIGFILE="/home/pengine/prod/live_configs/common_"$PROFILE"_ors.cfg" ;

  # Test if file is good, readable and non-zero size 
    [ -f $CONFIGFILE -a -s $CONFIGFILE -a -r $CONFIGFILE ] || print_msg_and_exit "ORS_CONFIG_FILE -> $CONFIGFILE EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/ORSlogs/PID_ORS_DIR/$EXCH"_"$PROFILE"_"PIDfile.txt ;
    OUTPUTLOGDIR=/spare/local/ORSlogs/$EXCH/$PROFILE/ ;

    logfile=$OUTPUTLOGDIR"log.`date "+%Y%m%d"`"
    positionfile=$OUTPUTLOGDIR"position.`date "+%Y%m%d"`"
    tradesfile=$OUTPUTLOGDIR"trades.`date "+%Y%m%d"`"

    load_exchange_specific_settings $EXCH ;
    update_system_settings ;

    [ ! "$CMD" == "START" ] || start_dc_ors_process ;
    [ ! "$CMD" == "STOP" ] || stop_dc_ors_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

####Initialize params and arguments required for CombinedSource
init_combinedsource () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    CW_EXEC="/home/pengine/prod/live_execs/CombinedShmWriter" ;

    [ -f $CW_EXEC ] || print_msg_and_exit "EXEC -> $CW_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    COMBINED_WRITER_CFG="/home/pengine/prod/live_configs/"`hostname`"_combinedwriter.cfg" ;

  # Test if file is good, readable and non-zero size 
    [ -f $COMBINED_WRITER_CFG -a -s $COMBINED_WRITER_CFG -a -r $COMBINED_WRITER_CFG ] || print_msg_and_exit "CW_CONFIG_FILE -> $COMBINED_WRITER_CFG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"CombinedShmWriter_"PIDfile.txt ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ;

    update_system_settings ;
    load_msg_accelerator "CSW" "OVERRIDE";

    logfile=$OUTPUTLOGDIR"combined_writer_log.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_combinedsource_process ; 
    [ ! "$CMD" == "STOP" ] || stop_combinedsource_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

####Initialize params and arguments required for mktDD
init_datadaemon () {

    [ $# -gt 2 ] || print_msg_and_exit "Usage : < script > < CMD > < EXCH > < MODE - REFERENCE/DATA/LOGGER/NCLOGGER > < DATA-OPT BCAST_IP & OPT BCAST_PORT / LOGGER-OPT LOG_DIR >" ;

    MKTDD_EXEC="/home/pengine/prod/live_execs/mktDD" ;

    [ -f $MKTDD_EXEC ] || print_msg_and_exit "EXEC -> $MKTDD_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    EXCH=$2 ;
    MODE=$3 ;

  # Test if file is good, readable and non-zero size 
    [ -f $MKTDD_EXEC -a -s $MKTDD_EXEC -a -r $MKTDD_EXEC ] || print_msg_and_exit "MKTDD EXEC  -> $MKTDD_EXEC EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"MKTDD_"$EXCH"_"$MODE"_PIDfile.txt" ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ;

    update_system_settings ;
    load_msg_accelerator "MKTDD" ;

    logfile=$OUTPUTLOGDIR"mktdd_"$EXCH"_"$MODE"."`date "+%Y%m%d"`".coutcerr" ;

    [ ! "$CMD" == "START" ] || start_datadaemon_process $* ;
    [ ! "$CMD" == "STOP" ] || stop_datadaemon_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

####Initialize params and arguments required for mktDD
init_sgxdatadaemon () {

    [ $# -gt 1 ] || print_msg_and_exit "Usage : < script > < CMD > < EXCH > " ;

    MKTDD_EXEC="/home/pengine/prod/live_execs/SGXDD" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $MKTDD_EXEC ] || print_msg_and_exit "EXEC -> $MKTDD_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    EXCH=$2 ;

  # Test if file is good, readable and non-zero size 
    [ -f $MKTDD_EXEC -a -s $MKTDD_EXEC -a -r $MKTDD_EXEC ] || print_msg_and_exit "MKTDD EXEC  -> $MKTDD_EXEC EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;
    [ -f $AFFIN_EXEC -a -s $AFFIN_EXEC -a -r $AFFIN_EXEC ] || print_msg_and_exit "MKTDD EXEC  -> $AFFIN_EXEC EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"MKTDD_"$EXCH"_PIDfile.txt" ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ;

    update_system_settings ;
    load_msg_accelerator "SGXDD" ;

    logfile=$OUTPUTLOGDIR"mktdd_"$EXCH"."`date "+%Y%m%d"`".coutcerr" ;
    SGX_MKTDD_CFG="/home/pengine/prod/live_configs/"`hostname`"_mktdata.cfg" ;

    [ ! "$CMD" == "START" ] || start_sgxdatadaemon_process $* ;
    [ ! "$CMD" == "STOP" ] || stop_sgxdatadaemon_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

####Initialize params and arguments required for CombinedSourceMulitcaster
init_combinedmulticaster () {

    [ $# -ge 1 ] || print_msg_and_exit "Usage : < script > < CMD > < EXCHANGE > < PRODUCTS-FILTER > OR Usage : < script > < CMD > < START/STOP >" ;

    CM_EXEC="/home/pengine/prod/live_execs/CombinedShmMulticaster" ;

    [ -f $CM_EXEC ] || print_msg_and_exit "EXEC -> $CM_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    EXCH="CONFIG" ;
    if [ $# -ge 2 ]; then
	EXCH=$2 ;
	PRODUCTS_FILTER="";
	if [ $# -eq 3 ]; then
            PRODUCTS_FILTER=$3;
	fi
    fi

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"CombinedShmMulticaster_"$EXCH"_PIDfile.txt" ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ;

    update_system_settings ;
    load_msg_accelerator "CSW" ;

    logfile=$OUTPUTLOGDIR"combined_multicaster_"$EXCH"_log.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_combinedmulticaster_process ; 
    [ ! "$CMD" == "STOP" ] || stop_combinedmulticaster_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

####Initialize params and arguments required for l1_sender
init_l1_sender () {

    [ $# -ge 2 ] || print_msg_and_exit "Usage : < script > < CMD > < EXCHANGE > < CONFIG_FILE >" ;

    CM_EXEC="/home/pengine/prod/live_execs/l1_sender" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $CM_EXEC ] || print_msg_and_exit "EXEC -> $CM_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    EXCH=$2 ;
    CONFIG_FILE=$3 ;

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"L1Sender_"$EXCH"_PIDfile.txt" ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ;

    update_system_settings ;
    load_msg_accelerator ;

    logfile=$OUTPUTLOGDIR"l1_sender_"$EXCH"_log.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_l1_sender_process ; 
    [ ! "$CMD" == "STOP" ] || stop_l1_sender_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

####Initialize params and arguments required for real_shm_packets_logger
init_realshmpacketslogger () { 

    [ $# -gt 0 ] || print_msg_and_exit "Usage : some usage..." ;

    REALSHMPACKETS_LOGGER_EXEC="/home/pengine/prod/live_execs/real_packets_logger" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $REALSHMPACKETS_LOGGER_EXEC ] || print_msg_and_exit "EXEC -> $REALSHMPACKETS_LOGGER_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ;

  # Test if file is good, readable and non-zero size 
    [ -f $REALSHMPACKETS_LOGGER_EXEC -a -s $REALSHMPACKETS_LOGGER_EXEC -a -r $REALSHMPACKETS_LOGGER_EXEC ] || print_msg_and_exit "REALSHMPACKETSLOGGER EXEC  -> $REALSHMPACKETS_LOGGER_EXEC EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;
    [ -f $AFFIN_EXEC -a -s $AFFIN_EXEC -a -r $AFFIN_EXEC ] || print_msg_and_exit "REALSHMPACKETSLOGGER EXEC  -> $AFFIN_EXEC EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/MDSlogs/PID_MDS_DIR/"REALSHMPACKETSLOGGER_PIDfile.txt" ;
    OUTPUTLOGDIR=/spare/local/MDSlogs/ ;

    update_system_settings ;

    logfile=$OUTPUTLOGDIR"realshmpacketslogger_"`date "+%Y%m%d"`".coutcerr" ;

    [ ! "$CMD" == "START" ] || start_realshmpacketslogger_process $* ; 
    [ ! "$CMD" == "STOP" ] || stop_realshmpacketslogger_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_nse_pairtrading_process () {
    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $PT_EXEC WITH CONFIG FILE $PAIR_TRADING_MULTPARAM_CFG SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    PAIR_TRADING_MULTPARAM_CFG="/home/dvctrader/LiveConfigs/PairTradingConfigs/mult_param_"$PROC_NO".cfg" ;

  # Test if file is good, readable and non-zero size 
    [ -f $PAIR_TRADING_MULTPARAM_CFG -a -s $PAIR_TRADING_MULTPARAM_CFG -a -r $PAIR_TRADING_MULTPARAM_CFG ] || print_msg_and_exit "PAIR_TRADING_MULTPARAM_CFG -> $PAIR_TRADING_MULTPARAM_CFG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    NSE_FO_DATA_CONVERTER="/home/pengine/prod/live_execs/nse_generic_to_dotex_offline_converter";
    [ -f $NSE_FO_DATA_CONVERTER -a -s $NSE_FO_DATA_CONVERTER -a -r $NSE_FO_DATA_CONVERTER ] || print_msg_and_exit "NSE_FO DATA CONVERTER -> $NSE_FO_DATA_CONVERTER EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    FILTERED_PRODUCT_LIST_FILE="/home/dvctrader/LiveConfigs/PairTradingConfigs/pair_trading_filtered_products_list_"$PROC_NO".txt"
    grep "INSTRUMENT_" `cat $PAIR_TRADING_MULTPARAM_CFG` | awk '{print $NF}'  | sort | uniq | awk '{print "NSE_"$1"_FUT0\nNSE_"$1"_FUT1"}' > $FILTERED_PRODUCT_LIST_FILE ;

    [ -f $FILTERED_PRODUCT_LIST_FILE -a -s $FILTERED_PRODUCT_LIST_FILE -a -r $FILTERED_PRODUCT_LIST_FILE ] || print_msg_and_exit "NSE_FO DATA FILTER LIST FILE -> $FILTERED_PRODUCT_LIST_FILE EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    
    today=`date +"%Y%m%d"` ;
    NSE_SPREAD_EXEC_SIM=/home/pengine/prod/live_execs/spread_exec_$PROC_NO ;

    DATE_EXEC=/home/pengine/prod/live_execs/update_date;
    echo "$DATE_EXEC $today P W"
    prevDate=`$DATE_EXEC $today P W`
    Folder='/spare/local/MDSlogs/MT_SPRD_SERIALIZED_DUMP/'
    
    #array of all relevant pairs
    pairs=$(cat $PAIR_TRADING_MULTPARAM_CFG | awk -F'/' '{print $NF}'| tr '~' '&');

    i=0;
    for p in $pairs; do if [ `find /spare/local/MDSlogs/MT_SPRD_SERIALIZED_DUMP/ -name "*$p"_"$today*" -mmin -20 -type f | wc -l` -eq 0 ] ; then ((i++)); fi; done

#IF WE NEED TO RUN SIMULATION THEN   ################## 
    if { false && [ `date +"%H%m"` -gt 355 ] && [ $i -gt 0 ] ; } ; then 
        
	[ -f $NSE_SPREAD_EXEC_SIM -a -s $NSE_SPREAD_EXEC_SIM -a -r $NSE_SPREAD_EXEC_SIM ] || print_msg_and_exit "NSE_SPREAD_EXEC_SIM -> $NSE_SPREAD_EXEC_SIM EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;
	
		#Starting Intraday Recovery
	DESTDIR="/NAS1/data/NSELoggedData/NSE/${today:0:4}/${today:4:2}/${today:6:2}/";
	mkdir -p $DESTDIR >>$logfile 2>&1		#create the destination dir
	for p in `cat $FILTERED_PRODUCT_LIST_FILE | awk -F"_" '{print $2}' | sort | uniq`; #get all underlying
	do 
	    str="/spare/local/MDSlogs/GENERIC/NSE_"$p"_FUT_*_"$today;
	    for file in `ls $str | tr ' ' '\n' | head -2`;		#gets the generic filenames for 1st and 2nd expiry 
	    do
		basefilename=`basename $file`;
		echo "$NSE_FO_DATA_CONVERTER $file $today $DESTDIR$basefilename" >>$logfile 2>&1
		$NSE_FO_DATA_CONVERTER $file $today $DESTDIR$basefilename >>$logfile 2>&1
	    done
	done;   

      #removing dates from prev day dated serialised dump
	for p in $pairs; do f=$Folder$p"_"$prevDate; newname=`echo $f | awk -F'/' '{print $(NF)}' | awk -F'_' '{print $1"_"$2"_"$3}'`; fullname=$Folder$newname;mv $f $fullname; done;

      #backup today's NAV series
	for p in $pairs; do f=/spare/local/MDSlogs/MT_SPRD_NAV_SERIES/$p"_"$today; mv $f $f".bkp"; done;

      #backup today's TRADES
	for p in $pairs; do f=/spare/local/MDSlogs/MT_SPRD_TRADES/$p"_"$today; mv $f $f".bkp"; done;

      #backup today's EXEC
	for p in $pairs; do f=/spare/local/MDSlogs/MT_SPRD_EXEC/$p"_"$today; mv $f $f".bkp"; done; 

      #run spread exec
	$NSE_SPREAD_EXEC_SIM --paramfile $PAIR_TRADING_MULTPARAM_CFG --start_date $today --end_date $today --use_exec_logic --hft_data --load_state --save_state --multiparam
	echo "$NSE_SPREAD_EXEC_SIM --paramfile $PAIR_TRADING_MULTPARAM_CFG --start_date $today --end_date $today --use_exec_logic --hft_data --load_state --save_state --multiparam"

      #renaming undated serialised dump to prevDay's dated serialised dump
	for p in $pairs; do f=$Folder$p; mv $f $f"_"$prevDate; done;

      #renaming NAV,TRADE and EXEC file so that live trader can append
	for p in $pairs; do f=/spare/local/MDSlogs/MT_SPRD_NAV_SERIES/$p; mv $f $f"_"$today; done;
	
	for p in $pairs; do f=/spare/local/MDSlogs/MT_SPRD_TRADES/$p; mv $f $f"_"$today; done;

	for p in $pairs; do f=/spare/local/MDSlogs/MT_SPRD_EXEC/$p; mv $f $f"_"$today; done;    

    else
      #renaming today's serialised dump to prevDay's dated serialised dump
	for p in $pairs; do f=$Folder$p"_"$today; newname=`echo $f | awk -F'/' '{print $(NF)}' | awk -F'_' '{print $1"_"$2"_"$3}'`; fullname=$Folder$newname'_'$prevDate;mv $f $fullname; done;
        
    fi
    
    ###########################################     

    echo "$ACCELERATOR_PREFIX $PT_EXEC --paramfile $PAIR_TRADING_MULTPARAM_CFG --start_date $today --end_date $today --use_exec_logic --live --hft_data --load_state --save_state --multiparam >>$logfile 2>&1 &"
    $ACCELERATOR_PREFIX $PT_EXEC --paramfile $PAIR_TRADING_MULTPARAM_CFG --start_date $today --end_date $today --use_exec_logic --live --hft_data --load_state --save_state --multiparam --qid $((567830+$PROC_NO)) --corp_adj_file $CORPADJFILE >>$logfile 2>&1 &
    PTPID=$! ;
    echo $PTPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $PTPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $PTPID "NSESpreadTrading" >>$logfile 2>&1
    exit ;

}

stop_nse_pairtrading_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $PT_EXEC WITH CONFIG FILE $PAIR_TRADING_MULTPARAM_CFG SINCE $PIDFILE DOESN'T EXISTS" ;

    PTPID=`tail -1 $PIDFILE` ;
    kill -2 $PTPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $PTPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $PTPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $PTPID" ;
            kill -9 $PTPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_nsepairtrading () {

    [ $# -eq 2 ] || print_msg_and_exit "Usage : < script > < process_number >  < CMD >" ;

    PROC_NO=$1
    PT_EXEC="/home/pengine/prod/live_execs/nse_spread_tradeinit_"$PROC_NO ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $PT_EXEC ] || print_msg_and_exit "EXEC -> $PT_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$2 ;

    PIDFILE=/spare/local/logs/tradelogs/PID_STRAT_DIR/NSEPairTradingPIDfile_$PROC_NO.txt ;
    OUTPUTLOGDIR=/spare/local/logs/tradelogs/queryoutput/ ;

    update_system_settings ;

    logfile=$OUTPUTLOGDIR"nse_pair_trading_"$PROC_NO"_log.`date "+%Y%m%d"`"
    CORPADJFILE="/home/dvctrader/LiveConfigs/PairTradingConfigs/corp_adj.txt"

    [ ! "$CMD" == "START" ] || start_nse_pairtrading_process ;
    [ ! "$CMD" == "STOP" ] || stop_nse_pairtrading_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_nse_simpletrading_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $PT_EXEC WITH CONFIG FILE $SIMPLE_TRADING_MULTPARAM_CFG SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;


    $ACCELERATOR_PREFIX $PT_EXEC $SIMPLE_TRADING_MULTPARAM_CFG FILE >>$logfile 2>&1 &
    PTPID=$! ;
    echo $PTPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $PTPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $PTPID "NSESimpleTrading" >>$logfile 2>&1
    
    exit ;

}

stop_nse_simpletrading_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $PT_EXEC WITH CONFIG FILE $SIMPLE_TRADING_MULTPARAM_CFG SINCE $PIDFILE DOESN'T EXISTS" ;

    PTPID=`tail -1 $PIDFILE` ;
    kill -2 $PTPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $PTPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $PTPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $PTPID" ;
            kill -9 $PTPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_nsesimpletrading () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    PT_EXEC="/home/pengine/prod/live_execs/nse_given_notional_tradeinit_Final" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $PT_EXEC ] || print_msg_and_exit "EXEC -> $PT_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    SIMPLE_TRADING_MULTPARAM_CFG="/home/dvctrader/LiveConfigs/NSE_SIMPLE_EXEC_PARAMS/mult_param_rv" ;

  # Test if file is good, readable and non-zero size 
    [ -f $SIMPLE_TRADING_MULTPARAM_CFG -a -s $SIMPLE_TRADING_MULTPARAM_CFG -a -r $SIMPLE_TRADING_MULTPARAM_CFG ] || print_msg_and_exit "SIMPLE_TRADING_MULTPARAM_CFG -> $SIMPLE_TRADING_MULTPARAM_CFG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/logs/tradelogs/PID_STRAT_DIR/NSESimpleTradingPIDfile.txt ;
    OUTPUTLOGDIR=/spare/local/logs/tradelogs/queryoutput/ ;

    update_system_settings ;

    logfile=$OUTPUTLOGDIR"nse_simple_exec_logic_.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_nse_simpletrading_process ;
    [ ! "$CMD" == "STOP" ] || stop_nse_simpletrading_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_nse_weeklyshortgamma_trading_process () {

    [ ! -f $WEEKLYSHORTGAMMA_PIDFILE -a ! -e $WEEKLYSHORTGAMMA_PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $WEEKLYSHORTGAMMA_EXEC WITH CONFIG FILE $WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG SINCE $WEEKLYSHORTGAMMA_PIDFILE ALREADY EXISTS" ;
    [ -d $WEEKLYSHORTGAMMA_OUTPUT_LOGDIR ] || mkdir -p $WEEKLYSHORTGAMMA_OUTPUT_LOGDIR ;

    [ ! -f $weeklyshortgamma_logfile ] || cp $weeklyshortgamma_logfile $weeklyshortgamma_logfile"_`date "+%s"`" ;


    $ACCELERATOR_PREFIX $WEEKLYSHORTGAMMA_EXEC $WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG TCP >>$weeklyshortgamma_logfile 2>&1 &
    WEEKLYSHORTGAMMA_PID=$! ;
    echo $WEEKLYSHORTGAMMA_PID > $WEEKLYSHORTGAMMA_PIDFILE ;

    [ -f $WEEKLYSHORTGAMMA_PIDFILE -a -r $WEEKLYSHORTGAMMA_PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $WEEKLYSHORTGAMMA_PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $WEEKLYSHORTGAMMA_PIDFILE -a -r $WEEKLYSHORTGAMMA_PIDFILE ] || kill -9 $WEEKLYSHORTGAMMA_PTPID ;
    [ -f $WEEKLYSHORTGAMMA_PIDFILE -a -r $WEEKLYSHORTGAMMA_PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $WEEKLYSHORTGAMMA_PID "NSEWeeklyshortgammaTrading" >>$weeklyshortgamma_logfile 2>&1

    exit ;

}

stop_nse_weeklyshortgamma_trading_process () {

    [ -f $WEEKLYSHORTGAMMA_PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $WEEKLYSHORTGAMMA_EXEC WITH CONFIG FILE $WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG SINCE $WEEKLYSHORTGAMMA_PIDFILE DOESN'T EXISTS" ;

    WEEKLYSHORTGAMMA_PID=`tail -1 $WEEKLYSHORTGAMMA_PIDFILE` ;
    kill -2 $WEEKLYSHORTGAMMA_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $WEEKLYSHORTGAMMA_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $WEEKLYSHORTGAMMA_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $WEEKLYSHORTGAMMA_PID" ;
            kill -9 $WEEKLYSHORTGAMMA_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $WEEKLYSHORTGAMMA_PIDFILE ;

    exit ;

}

init_nse_weeklyshortgamma_trading () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    WEEKLYSHORTGAMMA_EXEC="/home/pengine/prod/live_execs/nse_given_notional_tradeinit_Final" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $WEEKLYSHORTGAMMA_EXEC ] || print_msg_and_exit "EXEC -> $WEEKLYSHORTGAMMA_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG="/home/dvctrader/LiveConfigs/NSE_SIMPLE_EXEC_PARAMS/mult_param_weeklyshortgamma60" ;

  # Test if file is good, readable and non-zero size 
    [ -f $WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG -a -s $WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG -a -r $WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG ] || print_msg_and_exit "WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG -> $WEEKLYSHORTGAMMA_TRADING_MULTPARAM_CFG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    WEEKLYSHORTGAMMA_PIDFILE=/spare/local/logs/tradelogs/PID_STRAT_DIR/NSEWeeklyshortgammaTradingPIDfile.txt ;
    WEEKLYSHORTGAMMA_OUTPUT_LOGDIR=/spare/local/logs/tradelogs/queryoutput/ ;

    update_system_settings ;

    weeklyshortgamma_logfile=$WEEKLYSHORTGAMMA_OUTPUT_LOGDIR"nse_weeklyshortgamma_exec_logic_.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_nse_weeklyshortgamma_trading_process ;
    [ ! "$CMD" == "STOP" ] || stop_nse_weeklyshortgamma_trading_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}


start_oebu_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $OEBU_EXEC WITH CONFIG FILE $PAIR_TRADING_MULTPARAM_CFG SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $OEBU_EXEC $OEBU_CONFIG >$oebu_out_path 2>>$logfile &
    OEBUPID=$! ;
    echo $OEBUPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $OEBUPID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_oebu_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $OEBU_EXEC WITH CONFIG FILE $OEBU_CONFIG SINCE $PIDFILE DOESN'T EXISTS" ;

    OEBUPID=`tail -1 $PIDFILE` ;
    kill -2 $OEBUPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $OEBUPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $OEBUPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $OEBUPID" ;
            kill -9 $OEBUPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_oebu () {

    [ $# -eq 1 ] || [ $# -eq 2 ] || print_msg_and_exit "Usage : < script > < Optional-Arg for running multiple instance (1,2,3..) > < CMD >" ;

    OEBU_EXEC="/home/pengine/prod/live_execs/our_extended_bidask_mkt_book_util" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $OEBU_EXEC ] || print_msg_and_exit "EXEC -> $OEBU_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    if [ $# -eq 1 ]
    then
	CMD=$1 ;
	OEBU_CONFIG="/home/pengine/prod/live_configs/`hostname`_oebu_volmon_product_list.txt" ;
	PIDFILE=/spare/local/logs/EXEC_PID_DIR/OEBU_PIDFILE.txt
	logfile=$OUTPUTLOGDIR"oebu.`date "+%Y%m%d"`"
	oebu_out_path=oebu_out
    elif [ $# -eq 2 ]
    then
    	OEBU_INSTANCE=$1;
    	CMD=$2;
    	OEBU_CONFIG="/home/pengine/prod/live_configs/`hostname`_oebu_volmon_product_list_${OEBU_INSTANCE}.txt" ;
    	PIDFILE="/spare/local/logs/EXEC_PID_DIR/OEBU_PIDFILE${OEBU_INSTANCE}.txt"
    	logfile=$OUTPUTLOGDIR"oebu_${OEBU_INSTANCE}.`date "+%Y%m%d"`"
    	oebu_out_path="oebu_out_${OEBU_INSTANCE}"
    fi

  # Test if file is good, readable and non-zero size 
    [ -f $OEBU_CONFIG -a -s $OEBU_CONFIG -a -r $OEBU_CONFIG ] || print_msg_and_exit "OEBU_CONFIG -> $OEBU_CONFIG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    update_system_settings ;

    [ ! "$CMD" == "START" ] || start_oebu_process ;
    [ ! "$CMD" == "STOP" ] || stop_oebu_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

init_nse_oebu () {

    [ $# -eq 1 ] || [ $# -eq 2 ] || print_msg_and_exit "Usage : < script > < Optional-Arg for running multiple instance (1,2,3..) > < CMD >" ;

    OEBU_EXEC="/home/pengine/prod/live_execs/our_extended_bidask_mkt_book_util_nse" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $OEBU_EXEC ] || print_msg_and_exit "EXEC -> $OEBU_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    if [ $# -eq 1 ]
    then
	CMD=$1 ;
	OEBU_CONFIG="/spare/local/files/oebu_volmon_product_list.txt" ;
	PIDFILE=/spare/local/logs/EXEC_PID_DIR/OEBU_PIDFILE.txt
	logfile=$OUTPUTLOGDIR"nse_oebu.`date "+%Y%m%d"`"
	oebu_out_path=$HOME/trash/nse_oebu_out
    elif [ $# -eq 2 ]
    then
	OEBU_INSTANCE=$1;
	CMD=$2;
	OEBU_CONFIG="/spare/local/files/oebu_volmon_product_list_${OEBU_INSTANCE}.txt" ;
	PIDFILE="/spare/local/logs/EXEC_PID_DIR/NSE_OEBU_PIDFILE${OEBU_INSTANCE}.txt"
	logfile=$OUTPUTLOGDIR"nse_oebu_${OEBU_INSTANCE}.`date "+%Y%m%d"`"
	oebu_out_path="$HOME/trash/nse_oebu_out_${OEBU_INSTANCE}"
    fi

  # Test if file is good, readable and non-zero size
    [ -f $OEBU_CONFIG -a -s $OEBU_CONFIG -a -r $OEBU_CONFIG ] || print_msg_and_exit "OEBU_CONFIG -> $OEBU_CONFIG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    update_system_settings ;

    [ ! "$CMD" == "START" ] || start_oebu_process ;
    [ ! "$CMD" == "STOP" ] || stop_oebu_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_volmon_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $VOLMON_EXEC WITH CONFIG FILE $PAIR_TRADING_MULTPARAM_CFG SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $VOLMON_EXEC $VOLMON_CONFIG >>moving_volumes.txt 2>&1 &
    VOLMONPID=$! ;
    echo $VOLMONPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $VOLMONPID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_volmon_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $VOLMON_EXEC WITH CONFIG FILE $VOLMON_CONFIG SINCE $PIDFILE DOESN'T EXISTS" ;

    VOLMONPID=`tail -1 $PIDFILE` ;
    kill -2 $VOLMONPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $VOLMONPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $VOLMONPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $VOLMONPID" ;
            kill -9 $VOLMONPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_volmon () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    VOLMON_EXEC="/home/pengine/prod/live_execs/volume_monitor" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $VOLMON_EXEC ] || print_msg_and_exit "EXEC -> $VOLMON_EXEC DOESN'T EXIST" ;

    CMD=$1 ;
    VOLMON_CONFIG="/home/pengine/prod/live_configs/`hostname`_oebu_volmon_product_list.txt" ;

  # Test if file is good, readable and non-zero size 
    [ -f $VOLMON_CONFIG -a -s $VOLMON_CONFIG -a -r $VOLMON_CONFIG ] || print_msg_and_exit "VOLMON_CONFIG -> $VOLMON_CONFIG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/VOLMON_PIDFILE.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;

    update_system_settings ;

    logfile=$OUTPUTLOGDIR"oebu.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_volmon_process ;
    [ ! "$CMD" == "STOP" ] || stop_volmon_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_ors_data_logger_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $ORSDATALOGGER SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $ORSDATALOGGER >>$logfile 2>&1 &
    ORSDATALOGGERPID=$! ;
    echo $ORSDATALOGGERPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $ORSDATALOGGERPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_ors_data_logger_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $ORSDATALOGGER SINCE $PIDFILE DOESN'T EXISTS" ;

    ORSDATALOGGERPID=`tail -1 $PIDFILE` ;
    kill -2 $ORSDATALOGGERPID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $ORSDATALOGGERPID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $ORSDATALOGGERPID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $ORSDATALOGGERPID" ;
            kill -9 $ORSDATALOGGERPID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_ors_data_logger () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    ORSDATAEXEC="/home/pengine/prod/live_execs/smart_ors_data_logger" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $ORSDATAEXEC ] || print_msg_and_exit "EXEC -> $ORSDATAEXEC DOESN'T EXIST" ;

    CMD=$1 ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/ORSDATALOGGER_PIDFILE.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;

    update_system_settings ;
    load_msg_accelerator ;

    logfile=$OUTPUTLOGDIR"oebu.`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_ors_data_logger_process ;
    [ ! "$CMD" == "STOP" ] || stop_ors_data_logger_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

start_rmc_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $RMC_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $RMC_EXEC $ADDTRADESYM_CONF >> $logfile 2>&1 &
    RMC_PID=$! ;
    echo $RMC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $RMC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_rmc_process () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $RMC_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    RMC_PID=`tail -1 $PIDFILE` ;
    kill -2 $RMC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $RMC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $RMC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $RMC_PID" ;
            kill -9 $RMC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}
init_rmc () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    RMC_EXEC="/home/pengine/prod/live_execs/risk_monitor_client" ;
    ADDTRADESYM_CONF="/home/pengine/prod/live_configs/`hostname`_addts.cfg"
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $RMC_EXEC ] || print_msg_and_exit "EXEC -> $RMC_EXEC DOESN'T EXIST" ;

    CMD=$1 ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/RMC_PIDFILE.txt
    OUTPUTLOGDIR=/spare/local/logs/risk_logs/ ;

    update_system_settings ;

    logfile=$OUTPUTLOGDIR"risk_client_err_`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_rmc_process ;
    [ ! "$CMD" == "STOP" ] || stop_rmc_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}
start_alpes_gui_server() {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $ALPES_GUI_SERVER_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

    $ALPES_GUI_SERVER_EXEC --icap-config $ICAP_ORS_CONFIG >> $logfile 2>&1 &
    EXEC_PID=$! ;
    echo $EXEC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $EXEC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_alpes_gui_server() {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $ALPES_GUI_SERVER_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    EXEC_PID=`tail -1 $PIDFILE` ;
    kill -2 $EXEC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $EXEC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $EXEC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $EXEC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_alpes_gui_server () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    ALPES_GUI_SERVER_EXEC="/home/pengine/prod/live_execs/alpes_gui_server" ;
    ICAP_ORS_CONFIG="/spare/local/files/icap_ors.cfg";
    [ -f $ALPES_GUI_SERVER_EXEC ] || print_msg_and_exit "EXEC -> $ALPES_GUI_SERVER_EXEC DOESN'T EXIST" ;

    CMD=$1 ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/ALPES_GUI_PIDfile.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;

    logfile=$OUTPUTLOGDIR"alpes_gui.log_`date "+%Y%m%d"`"

    [ ! "$CMD" == "START" ] || start_alpes_gui_server ;
    [ ! "$CMD" == "STOP" ] || stop_alpes_gui_server ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;
}

start_fix_parser() {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $ORS_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;
	#this file is opened in append mode, but still making a copy
    if [ -f $tradesfile ] ;
    then
	cp $tradesfile $tradesfile"_`date "+%s"`"
    fi

    if [ "$ACR" == "ONLOAD" ] || [ "$ACR" == "ON" ]
    then

	onload $ORS_EXEC --alpes-config $ALPES_CONFIGFILE --icap-config $ICAP_CONFIGFILE --cmc-config $CMC_CONFIGFILE --xp-user1-config $XP_USER1_CONFIGFILE --xp-user2-config $XP_USER2_CONFIGFILE --ativa-config $ATIVA_CONFIGFILE --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE >>$COUT_CERR_FILE &

    elif [ "$ACR" == "VMA" ]
    then

        #dynamically linked lib
        LD_PRELOAD=libvma.so $ORS_EXEC --alpes-config $ALPES_CONFIGFILE --icap-config $ICAP_CONFIGFILE --cmc-config $CMC_CONFIGFILE --xp-user1-config $XP_USER1_CONFIGFILE --xp-user2-config $XP_USER2_CONFIGFILE --ativa-config $ATIVA_CONFIGFILE --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE >>$COUT_CERR_FILE &

    elif [ "$ACR" == "EXASOCK" ]
    then

	exasock $ORS_EXEC --alpes-config $ALPES_CONFIGFILE --icap-config $ICAP_CONFIGFILE --cmc-config $CMC_CONFIGFILE --xp-user1-config $XP_USER1_CONFIGFILE --xp-user2-config $XP_USER2_CONFIGFILE --ativa-config $ATIVA_CONFIGFILE --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE >>$COUT_CERR_FILE &

    else

	$ORS_EXEC --alpes-config $ALPES_CONFIGFILE --icap-config $ICAP_CONFIGFILE --cmc-config $CMC_CONFIGFILE --xp-user1-config $XP_USER1_CONFIGFILE --xp-user2-config $XP_USER2_CONFIGFILE --ativa-config $ATIVA_CONFIGFILE --output-log-dir $OUTPUTLOGDIR >>$COUT_CERR_FILE >>$COUT_CERR_FILE &

    fi

    ORSPID=$!
    echo $ORSPID > $PIDFILE

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $ORSPID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_fix_parser() {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $ORS_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    ORS_PID=`tail -1 $PIDFILE` ;
    kill -2 $ORS_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $ORS_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $ORS_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $ORS_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_fix_parser () {

    [ $# -ne 11 ] || print_msg_and_exit "USAGE: $0 EXCH ALPES_PROFILE ICAP_PROFILE XP_1 XP_2 CMC ATIVA CMD ACCELERATOR[ONLOAD(ON)/VMA/EXASOCK/OFF] POSFILE[KEEP/CLEAR]" ;

    ORS_EXEC="/home/pengine/prod/live_execs/alpes_gui_fix_parser" ;
    [ -f $ORS_EXEC ] || print_msg_and_exit "EXEC -> $ORS_EXEC DOESN'T EXIST" ;
    EXCH=$1; shift;
    ALPES_PROFILE=$1; shift;
    ICAP_PROFILE=$1 ; shift;
    XP_USER1_PROFILE=$1; shift ;
    XP_USER2_PROFILE=$1; shift ;
    CMC_PROFILE=$1 ; shift;
    ATIVA_PROFILE=$1 ; shift;
    CMD=$1 ; shift;
    ACR=$1; shift;
    POS=$1; shift;

    CONFIGDIR=$HOME/infracore_install/Configs/OrderRoutingServer/cfg ;
    PIDFILE=/spare/local/logs/EXEC_PID_DIR/FIX_PARSER_PIDfile.txt
    OUTPUTLOGDIR=/spare/local/ORSlogs/$EXCH/RETAIL/;

    logfile=logfile=$OUTPUTLOGDIR"log.`date "+%Y%m%d"`"
    positionfile=$OUTPUTLOGDIR"position.`date "+%Y%m%d"`"
    tradesfile=$OUTPUTLOGDIR"trades.`date "+%Y%m%d"`"

    COUT_CERR_FILE=/spare/local/logs/alllogs/ALPES_FIX_COUT_CERR.log

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
	export VMA_LOG_FILE=/spare/local/logs/alllogs/ORS_DEBUG.log
	export VMA_QP_LOGIC=0;
	export VMA_RX_POLL=-1;
	export VMA_SELECT_POLL=-1;
	export VMA_APPLICATION_ID="ORS";

    fi

    if [ "$ACR" == "EXASOCK" ]
    then
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/Exanic-1.8.1/ ; 
    fi

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/OAPI19.1/ ;

    if [ $PWD != $HOME ] ; then cd $HOME ; fi

    ALPES_CONFIGFILE=$CONFIGDIR/$ALPES_PROFILE/ors.cfg #note this need not be the same as $PROFILE. We could do a switch case like even if EXCH iS "CME" and PROFILE is "TEST" CONFIGFILE uses "8Q3"
    ICAP_CONFIGFILE=$CONFIGDIR/$ICAP_PROFILE/ors.cfg
    CMC_CONFIGFILE=$CONFIGDIR/$CMC_PROFILE/ors.cfg
    ATIVA_CONFIGFILE=$CONFIGDIR/$ATIVA_PROFILE/ors.cfg
    XP_USER1_CONFIGFILE=$CONFIGDIR/$XP_USER1_PROFILE/ors.cfg
    XP_USER2_CONFIGFILE=$CONFIGDIR/$XP_USER2_PROFILE/ors.cfg

    [ ! "$CMD" == "START" ] || start_fix_parser ;
    [ ! "$CMD" == "STOP" ] || stop_fix_parser ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;
}

start_smart_ors_data_logger() {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $SMART_ORS_DATA_LOGGER_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $SMART_ORS_DATA_LOGGER_EXEC >> $logfile 2>&1 &
    EXEC_PID=$! ;
    echo $EXEC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $EXEC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_smart_ors_data_logger() {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $SMART_ORS_DATA_LOGGER_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    EXEC_PID=`tail -1 $PIDFILE` ;
    kill -2 $EXEC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $EXEC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $EXEC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $EXEC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_smart_ors_data_logger () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    SMART_ORS_DATA_LOGGER_EXEC="/home/pengine/prod/live_execs/smart_ors_data_logger" ;
    [ -f $SMART_ORS_DATA_LOGGER_EXEC ] || print_msg_and_exit "EXEC -> $SMART_ORS_DATA_LOGGER_EXEC DOESN'T EXIST" ;

    CMD=$1 ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/SMART_ORS_DATA_LOGGER_PIDfile.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    
    update_system_settings ;
    load_msg_accelerator "CSW";
    
    logfile=$OUTPUTLOGDIR"smart_ors_data_logger.log_`date "+%Y%m%d"`"
    
    [ ! "$CMD" == "START" ] || start_smart_ors_data_logger ;
    [ ! "$CMD" == "STOP" ] || stop_smart_ors_data_logger ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;
}

start_cme_arbitrator() {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $CMEARB_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $CMEARB_EXEC >> $logfile 2>&1 &
    EXEC_PID=$! ;
    echo $EXEC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $EXEC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    $AFFIN_EXEC ASSIGN $EXEC_PID cme_ors_mkt_arb >>$logfile 2>&1

    exit ;
}

stop_cme_arbitrator() {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $CMEARB_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    EXEC_PID=`tail -1 $PIDFILE` ;
    kill -2 $EXEC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $EXEC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $EXEC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $EXEC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_cme_arbitrator () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    CMEARB_EXEC="/home/pengine/prod/live_execs/cme_ors_mkt_arb" ;
    [ -f $CMEARB_EXEC ] || print_msg_and_exit "EXEC -> $CMEARB_EXEC DOESN'T EXIST" ;

    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    CMD=$1 ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/CMEARB_PIDfile.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    
    update_system_settings ;
    load_msg_accelerator ;
    
    logfile=$OUTPUTLOGDIR"cmearb.log_`date "+%Y%m%d"`"
    
    [ ! "$CMD" == "START" ] || start_cme_arbitrator ;
    [ ! "$CMD" == "STOP" ] || stop_cme_arbitrator ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;
}

start_exch_sim() {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $EXCH_SIM_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $EXCH_SIM_EXEC $CONFIGFILE >> $logfile 2>&1 &
    EXEC_PID=$! ;
    echo $EXEC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $EXEC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    exit ;
}

stop_exch_sim() {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $EXCH_SIM_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    EXEC_PID=`tail -1 $PIDFILE` ;
    kill -2 $EXEC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $EXEC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $EXEC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $EXEC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_exch_sim () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    EXCH_SIM_EXEC="/home/dvcinfra/important/exchange_simulator" ;
    [ -f $EXCH_SIM_EXEC ] || print_msg_and_exit "EXEC -> $SEXCH_SIM_EXEC DOESN'T EXIST" ;

    CMD=$1 ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/EXCH_SIM_EXEC_PIDfile.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    
    CONFIGFILE=/home/dvcinfra/important/input.txt 

    update_system_settings ;
    load_msg_accelerator ;
    
    logfile=$OUTPUTLOGDIR"exch_sim.log_`date "+%Y%m%d"`"
    
    [ ! "$CMD" == "START" ] || start_exch_sim ;
    [ ! "$CMD" == "STOP" ] || stop_exch_sim ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;
}

start_cme_fpga_data_logger() {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $CME_FPGA_DATA_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

    $CME_FPGA_DATA_EXEC >> $logfile 2>&1 &
    EXEC_PID=$! ;
    echo $EXEC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $EXEC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;
    
    $AFFIN_EXEC ASSIGN $EXEC_PID $CME_FPGA_DATA_EXEC >>$logfile 2>&1
    
    exit ;
}

stop_cme_fpga_data_logger() {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $CME_FPGA_DATA_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    EXEC_PID=`tail -1 $PIDFILE` ;
    kill -2 $EXEC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $EXEC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $EXEC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $EXEC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;

}

init_cme_fpga_data_logger () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    CME_FPGA_DATA_EXEC="/home/pengine/prod/live_execs/cme_fpga_data_logger" ;
    [ -f $CME_FPGA_DATA_EXEC ] || print_msg_and_exit "EXEC -> $CME_FPGA_DATA_EXEC DOESN'T EXIST" ;
    
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;
    
    CMD=$1 ;

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/CME_FPGA_DATA_EXEC_PIDfile.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    
    update_system_settings ;
    
    logfile=$OUTPUTLOGDIR"cme_fpga_data_logger.log_`date "+%Y%m%d"`"
    
    [ ! "$CMD" == "START" ] || start_cme_fpga_data_logger ;
    [ ! "$CMD" == "STOP" ] || stop_cme_fpga_data_logger ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;
}

start_tbt_recovery_manager () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $TBT_RM_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

    $ACCELERATOR_PREFIX $TBT_RM_EXEC $EXCHANGE >> $logfile 2>&1 &
    EXEC_PID=$! ;
    echo $EXEC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $EXEC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;
    
    $AFFIN_EXEC ASSIGN $EXEC_PID "NSE_TBT_RM" >>$logfile 2>&1
    
    exit ;
}

stop_tbt_recovery_manager () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $TBT_RM_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    EXEC_PID=`tail -1 $PIDFILE` ;
    kill -2 $EXEC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $EXEC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $EXEC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $EXEC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;
}

init_tbt_recovery_manager () {

    [ $# -ge 1 ] || print_msg_and_exit "Usage : < script > < CMD > < OPT - EXCHANGE >" ;

    TBT_RM_EXEC="/home/pengine/prod/live_execs/nse_tbt_recovery_manager" ;
    [ -f $TBT_RM_EXEC ] || print_msg_and_exit "EXEC -> $TBT_RM_EXEC DOESN'T EXIST" ;
    
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;
    
    CMD=$1 ;
    EXCHANGE=$2;
    STKNAME="TBTRM" ;

    if [ "$EXCHANGE" == "NSE_FO" ] ; then
      STKNAME="TBTRMFO" ;
    fi
    if [ "$EXCHANGE" == "NSE_CD" ] ; then
      STKNAME="TBTRMCD" ;
    fi
    if [ "$EXCHANGE" == "NSE_EQ" ] ; then
      STKNAME="TBTRMEQ" ;
    fi


    PIDFILE=/spare/local/logs/EXEC_PID_DIR/TBT_RM_EXEC_$EXCHANGE"_PIDfile.txt"
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    
    update_system_settings ;
    load_msg_accelerator "$STKNAME" "OVERRIDE" ;
    logfile=$OUTPUTLOGDIR"tbt_recovery_manager.log_`date "+%Y%m%d"`"_$EXCHANGE ;
    
    [ ! "$CMD" == "START" ] || start_tbt_recovery_manager ;
    [ ! "$CMD" == "STOP" ] || stop_tbt_recovery_manager ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}


start_bmf_fpga_daemon () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTANCE OF $BMF_FPGA_DATA_EXEC SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    dt=`date "+%Y%m%d"` ;

	$ACCELERATOR_PREFIX $BMF_FPGA_DATA_EXEC --config "/spare/local/files/BMF/fpga_puma.cfg" --mode $MODE >> $logfile 2>&1 &
    EXEC_PID=$! ;
    echo $EXEC_PID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $EXEC_PID;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;
            
    exit ;
}

stop_bmf_fpga_daemon () {

    [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTANCE OF $BMF_FPGA_DATA_EXEC SINCE $PIDFILE DOESN'T EXISTS" ;
    EXEC_PID=`tail -1 $PIDFILE` ;
    kill -2 $EXEC_PID >/dev/null 2>/dev/null ;

    running_proc_string=`ps -p $EXEC_PID -o comm=`;

    if [ $running_proc_string ] ; then
        sleep 15 ;
        running_proc_string=`ps -p $EXEC_PID -o comm=`;
        if [ $running_proc_string ] ; then
            echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $EXEC_PID" ;
            kill -9 $EXEC_PID >/dev/null 2>/dev/null ;
        fi
    fi

    rm -rf $PIDFILE ;

    exit ;
}

init_bmf_fpga () {

    [ $# -eq 2 ] || print_msg_and_exit "Usage : < script > < CMD > < MODE > " ;

    BMF_FPGA_DATA_EXEC="/home/pengine/prod/live_execs/bmf_fpga_data_daemon" ;
    [ -f $BMF_FPGA_DATA_EXEC ] || print_msg_and_exit "EXEC -> $BMF_FPGA_DATA_EXEC_EXEC DOESN'T EXIST" ;   
 
    CMD=$1 ;
    MODE=$2

    PIDFILE=/spare/local/logs/EXEC_PID_DIR/BMF_FPGA_DATA_EXEC_"$MODE"_PIDfile.txt
    OUTPUTLOGDIR=/spare/local/logs/alllogs/ ;
    
    update_system_settings ;
    logfile=$OUTPUTLOGDIR"bmf_fpga_data_daemon_$MODE.log_`date "+%Y%m%d"`"
    
    [ ! "$CMD" == "START" ] || start_bmf_fpga_daemon ;
    [ ! "$CMD" == "STOP" ] || stop_bmf_fpga_daemon ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

init () {

    [ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < DAEMON_YOU_WANT_TO_CONTROL ( ORS/CombinedSource/DataDaemon/InteDataDaemon/CombinedMulticaster/OEBU/VOLMON/NSEPairTrading/NSESimpleTrading ) - Specific Daemon Will Print Their Usage If Required >" ; 

    DAEMON_SELECTED=$1 ; shift ;

    case $DAEMON_SELECTED in 

	ORS)
	    init_ors $* 
	    ;;
	CombinedSource)
	    init_combinedsource $* 
	    ;;
	DataDaemon) 
	    init_datadaemon $*
	    ;;
	SGXDataDaemon)
	    init_sgxdatadaemon $*
	    ;;
	CombinedMulticaster) 
	    init_combinedmulticaster $*
	    ;;
	L1Sender) 
	    init_l1_sender $*
	    ;;
	InteDataDaemon)
	    init_integrateddaemon $*
	    ;;
	RealShmPacketsLogger)
	    init_realshmpacketslogger $*
	    ;;
	OEBU)
	    init_oebu $*
	    ;;
	OEBU_NSE)
	    init_nse_oebu $*
	    ;;
	VOLMON)
	    init_volmon $*
	    ;;
	NSEPairTrading)
	    init_nsepairtrading $*
	    ;;
	NSESimpleTrading)
	    init_nsesimpletrading $*
	    ;;
	NSEWeeklyshortgammaTrading)
	    init_nse_weeklyshortgamma_trading $*
	    ;;
	ORSDataLogger)
	    init_ors_data_logger $*
	    ;;
	RMC)
	    init_rmc $*
	    ;;
	ALPES_GUI_SERVER)
	    init_alpes_gui_server $*
	    ;;
	RETAIL_FIX_PARSER)
	    init_fix_parser $*
	    ;;
	DCORS)
	    init_dc_ors $* 
	    ;;
	ORS_LOGGER)
	    init_smart_ors_data_logger $*
	    ;;
	CME_ARBITRATOR)
	    init_cme_arbitrator $*
	    ;;
	EXCH_SIM)
	    init_exch_sim $*
	    ;;
	CME_FPGA_LOGGER)
	    init_cme_fpga_data_logger $*
	    ;;
	TBT_RM)
	    init_tbt_recovery_manager $*
	    ;;
	BMF_FPGA)
	    init_bmf_fpga $*
	    ;;
	*)
	    print_msg_and_exit "Usage : < script > < DAEMON_YOU_WANT_TO_CONTROL ( ORS/CombinedSource/DataDaemon/InteDataDaemon/CombinedMulticaster/L1Sender/OEBU/VOLMON/NSEPairTrading/NSESimpleTrading/ORSBinLogger/DCORS/TBT_RM ) - Specific Daemon Will Print Their Usage If Required >" ;
    esac   

}

init $*
