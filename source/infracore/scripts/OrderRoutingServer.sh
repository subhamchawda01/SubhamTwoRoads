#!/bin/bash

print_msg_and_exit () {

  echo $* ; 
  exit ; 

}

update_system_settings () {

  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/spare/local/lib/OAPI19.1/ ;
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

####Initialize params and arguments required for ORS

init () {

  [ $# -eq 3 ] || print_msg_and_exit "Usage : < script > < PROFILE > < CMD > < POSFILE ( KEEP / CLEAR ) >" ; 

  ORS_EXEC="/home/pengine/prod/live_execs/cme_ilink_ors" ; 
  ORS_COMMAND_EXEC="/home/pengine/prod/live_scripts/ors_control.pl" ;

  [ -f $ORS_EXEC ] || print_msg_and_exit "ORS EXEC -> $ORS_EXEC DOESN'T EXIST" ;
  [ -f $ORS_COMMAND_EXEC ] || print_msg_and_exit "ORS CONTROL EXEC -> $ORS_COMMAND_EXEC DOESN'T EXIST" ;

  PROFILE=$1 ; shift ; 
  CMD=$1 ; shift ;
  POS=$1 ; 

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
  load_msg_accelerator ; 

  [ ! "$CMD" == "START" ] || start_process ; 
  [ ! "$CMD" == "STOP" ] || stop_process ; 

  print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ; 

}

start_and_login () {

  ADD_TS_SCRIPT=/home/dvcinfra/LiveExec/scripts/ADDTRADINGSYMBOL.sh
  ADD_TS_CONFIG=/home/dvcinfra/LiveExec/config/AddTradingSymbolConfig

  if [ "$EXCH" == "LIFFE" ] ; then 

    sleep 5 ; $ORS_COMMAND_EXEC $EXCH $PROFILE START >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ; 
    sleep 1 ; $ORS_COMMAND_EXEC $EXCH $PROFILE LOGIN >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ; 

  else 

    sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE START >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ; 
    sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE LOGIN >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ; 

  fi 

  $ADD_TS_SCRIPT $ADD_TS_CONFIG >/dev/null 2>&1 ;

}

logout_and_stop () {

  sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE LOGOUT >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;
  sleep 10 ; $ORS_COMMAND_EXEC $EXCH $PROFILE STOP >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 ;

}

start_process () {

  [ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $ORS_EXEC WITH CONFIG FILE $ORS_CONFIG_FILE SINCE $PIDFILE ALREADY EXISTS" ;
  [ -d $OUTPUTLOGDIR ] || mkdir -p $OUTPUTLOGDIR ; 

  [ ! -f $logfile ] || cp $logfile $logfile"_`date "+%s"`" ; 
  [ ! -f $tradesfile ] || cp $tradesfile $tradesfile"_`date "+%s"`" ; 

  [ "$POS" == "KEEP" ] || rm $positionfile ; 

  $ACCELERATOR_PREFIX $ORS_EXEC --config $CONFIGFILE --output-log-dir $OUTPUTLOGDIR >> $OUTPUTLOGDIR/cme_ilink_ors.COUT.CERR$TODAY 2>&1 &
  ORSPID=$! ; 
  echo $ORSPID > $PIDFILE ; 

  start_and_login ; 

  exit ; 

}

stop_process () {

  [ -f $PIDFILE ] || print_msg_and_exit "CANNOT STOP AN INSTNACE OF $ORS_EXEC WITH CONFIG FILE $ORS_CONFIG_FILE SINCE $PIDFILE DOESN'T EXISTS" ;

  logout_and_stop ; 

  ORSPID=`tail -1 $PIDFILE` ; 
  kill -2 $ORSPID >/dev/null 2>/dev/null ; 

  running_proc_string=`ps -p $ORSPID -o comm=`; 

  if [ $running_proc_string ] ; then 
    sleep 15 ; 
    running_proc_string=`ps -p $ORSPID -o comm=`;  
    if [ $running_proc_string ] ; then  
    echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $ORSPID" ;
    kill -9 $ORSPID >/dev/null 2>/dev/null ; 
    fi 
  fi 

  rm -rf $PIDFILE ; 

  exit ; 

}

init $*
