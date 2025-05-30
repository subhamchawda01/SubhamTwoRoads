#!/bin/bash
# Description : This script acts as a control mechanism to start and stop nse_tbt_recovery_manager 
# Author : Ravi 

print_msg_and_exit () {

  echo $* ;
  exit ;

}

update_system_settings () {

  ulimit -c unlimited
  if [ $PWD != $HOME ] ; then cd $HOME ; fi

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

####Initialize params and arguments required for NSE TBT Recovery Host 

init () {

    [ $# -eq 3 ] || print_msg_and_exit "Usage : < script > < CMD >" ;

    NSE_TBT_EXC="/home/pengine/prod/live_execs/nse_tbt_recovery_manager" ;

    [ -f $ORS_EXEC ] || print_msg_and_exit "NSE TBT RECOVERY HOST EXEC -> $NSE_TBT_EXC DOESN'T EXIST" ;

    CMD=$1 ; shift ;
    PIDFILE=/spare/local/common_pid_dir/NSE_TBT_RECOVERY_HOST_PIDFILE.txt ; 

    update_system_settings ;
    load_msg_accelerator ; 


    [ ! "$CMD" == "START" ] || start_process ;
    [ ! "$CMD" == "STOP" ] || stop_process ;

    print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ; 

}

start_process () {

  [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $NSE_TBT_EXC SINCE $PIDFILE ALREADY EXISTS" ;

  $ACCELERATOR_PREFIX $NSE_TBT_EXC >/dev/null 2>/dev/null &
  NTRPID=$! ; 

  echo $NTRPID > $PIDFILE ; 

  [ -f $PIDFILE -a -r $PIDFILE ] || echo "COULD NOT CREATE PID FILE.. -> $PIDFILE TERMINATING THE PROCESS..." ;
  [ -f $PIDFILE -a -r $PIDFILE ] || kill -9 $NTRPID ;
  [ -f $PIDFILE -a -r $PIDFILE ] || exit ;

  AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
  AF_FLAG=`cat $AF_FLAG_FILE` ;

  [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $NTRPID >>$logfile 2>&1
  
  exit ; 

}

stop_process () {

  [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $NSE_TBT_EXC SINCE $PIDFILE DOESN'T EXISTS" ; 

  NTRPID=`tail -1 $PIDFILE` ;
  kill -2 $NTRPID >/dev/null 2>/dev/null ;

  running_proc_string=`ps -p $NTRPID -o comm=`;

  if [ $running_proc_string ] ; then
    sleep 15 ;
    running_proc_string=`ps -p $NTRPID -o comm=`;
    if [ $running_proc_string ] ; then
    echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $NTRPID" ;
    kill -9 $NTRPID >/dev/null 2>/dev/null ;
    fi
  fi

  rm -rf $PIDFILE ;

  exit ;

}

init $*
