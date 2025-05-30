#!/bin/bash

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


start_nse_disp_process () {

    [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $PT_EXEC WITH CONFIG FILE $SIMPLE_TRADING_MULTPARAM_CFG SINCE $PIDFILE ALREADY EXISTS" ;
    [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ;

    [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ;


    LD_PRELOAD=important/libcrypto.so.1.1 $PT_EXEC $SIMPLE_TRADING_MULTPARAM_CFG FILE >>$logfile 2>&1 &
    PTPID=$! ;
    echo $PTPID > $PIDFILE ;

    [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
    [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $PTPID ;
    [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

    AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
    AF_FLAG=`cat $AF_FLAG_FILE` ;

    [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $PTPID "NSEDispTrading" >>$logfile 2>&1

    exit ;

}

stop_nse_disp_process () {

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

init_nsedisptrading () {

    [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    PT_EXEC="/home/pengine/prod/live_execs/nse_given_notional_tradeinit_disp" ;
    AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;

    [ -f $PT_EXEC ] || print_msg_and_exit "EXEC -> $PT_EXEC DOESN'T EXIST" ;
    [ -f $AFFIN_EXEC ] || print_msg_and_exit "EXEC -> $AFFIN_EXEC DOESN'T EXIST" ;

    export LD_LIBRARY_PATH=/opt/glibc-2.14/lib
    CMD=$1 ;
    SIMPLE_TRADING_MULTPARAM_CFG="/home/dvctrader/LiveConfigs/NSE_SIMPLE_EXEC_PARAMS/mult_param_disp" ;

  # Test if file is good, readable and non-zero size 
    [ -f $SIMPLE_TRADING_MULTPARAM_CFG -a -s $SIMPLE_TRADING_MULTPARAM_CFG -a -r $SIMPLE_TRADING_MULTPARAM_CFG ] || print_msg_and_exit "SIMPLE_TRADING_MULTPARAM_CFG -> $SIMPLE_TRADING_MULTPARAM_CFG EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

    PIDFILE=/spare/local/logs/tradelogs/PID_STRAT_DIR/NSEDispTradingPIDfile.txt ;
    OUTPUTLOGDIR=/spare/local/logs/tradelogs/queryoutput/ ;

    update_system_settings ;

    logfile=$OUTPUTLOGDIR"nse_disp_exec_logic_.`date "+%Y%m%d"`"
     echo "$CMD"
    [ ! "$CMD" == "START" ] || start_nse_disp_process ;
    [ ! "$CMD" == "STOP" ] || stop_nse_disp_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;

}

YYYYMMDD=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

init_nsedisptrading $*
