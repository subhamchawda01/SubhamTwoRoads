# . ~/LiveExec/scripts/make_config_for_dump.sh
# . ~/LiveExec/scripts/archive_raw_files_from_prod_servers.sh

init(){
	[ $# -gt 2 ] || print_msg_and_exit "Usage : < script > < CMD > < Exchange_name > <[stream_types]>" ;
	
	CMD=$1 ; shift ;
	MAKE_CONFIG_SCRIPT="/home/dvcinfra/LiveExec/scripts/make_config_for_dump.sh"
	RAW_DATA_LOG_EXEC="/home/dvcinfra/LiveExec/bin/dump_raw_data_new"
	#ARCHIVE_RAW_DATA_SCRIPT="/home/dvcinfra/LiveExec/scripts/archive_raw_files_from_prod_servers.sh"
	PIDFILE="/spare/local/MDSlogs/PID_MDS_DIR/${1}_raw_data_PIDfile.txt"
	
	[ -f $MAKE_CONFIG_SCRIPT ] || print_msg_and_exit "MAKE CONFIG SCRIPT -> $MAKE_CONFIG_SCRIPT DOESN'T EXIST" ;
 	#[ -f $ARCHIVE_RAW_DATA_SCRIPT ] || print_msg_and_exit "ARCHIVE RAW DATA SCRIPT -> $ARCHIVE_RAW_DATA_SCRIPT DOESN'T EXIST" ;
	[ -f $RAW_DATA_LOG_EXEC ] || print_msg_and_exit "RAW DATA LOG EXEC -> $RAW_DATA_LOG_EXEC DOESN'T EXIST" ;
 
  	load_msg_accelerator;
	
	[ ! "$CMD" == "START" ] || start_process $*;
  	[ ! "$CMD" == "STOP" ] || stop_process $*;

  	print_msg_and_exit "CMD SPECIFIED -> $CMD DOESN'T MAKE SENSE" ;
}
start_process () {
	[ ! -f $PIDFILE -a ! -e $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $MAKE_CONFIG_SCRIPT SINCE $PIDFILE ALREADY EXISTS" ;
	
	$MAKE_CONFIG_SCRIPT $*
	$ACCELERATOR_PREFIX $RAW_DATA_LOG_EXEC $1 > /dev/null 2>&1 &
	RAWPID=$! ;
  	echo $RAWPID > $PIDFILE ;
  	
  	exit ;
}
stop_process () {
	[ -f $PIDFILE ] || print_msg_and_exit "CANNOT START AN INSTNACE OF $ARCHIVE_RAW_DATA_SCRIPT SINCE $PIDFILE DOESN'T EXISTS" ;
	RAWPID=`tail -1 $PIDFILE` ;
  	kill -2 $RAWPID >/dev/null 2>/dev/null ;

 	running_proc_string=`ps -p $RAWPID -o comm=`;

  	if [ $running_proc_string ] ; then
    	sleep 1m ;
    	running_proc_string=`ps -p $RAWPID -o comm=`;
    	if [ $running_proc_string ] ; then
    		echo "SENDING SIGKILL AS PROCESS FAILED TO SHUTDOWN ON SIGINT, PID -> $RAWPID" ;
    		kill -9 $RAWPID >/dev/null 2>/dev/null ;
    	fi
  	fi
  rm -rf $PIDFILE ;
  
  #sleep 1m 
  
  #$ARCHIVE_RAW_DATA_SCRIPT $1
  
  exit;
	
}

print_msg_and_exit () {
  echo $* ;
  exit ;
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
init $*
