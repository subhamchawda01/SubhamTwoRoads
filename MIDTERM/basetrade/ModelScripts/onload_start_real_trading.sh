#!/bin/bash

USAGE="$0 EXCH PROGID STRATEGYDESCFILENAME ONLOAD[ON/OFF] AFFINITY[AF/NF] VMA[ON/OFF]";
if [ $# -lt 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRADE_EXEC=/home/pengine/prod/live_execs/tradeinit
AFFIN_EXEC=/home/pengine/prod/live_execs/cpu_affinity_mgr

EXCH=$1; shift;
PROGID=$1; shift;
STRATEGYDESCFILENAME=$1; shift;

ONL="OFF";

if [ $# -ge 1 ] ;
then
    ONL=$1; shift;
fi


AFO="NF";

if [ $# -ge 1 ] ;
then
    AFO=$1; shift;
fi

VMA="OFF";

if [ $# -ge 1 ] ;
then
    VMA=$1; shift;
fi

#for risk monitor computations
GIVEN_RISK_TAGS="GLOBAL";

if [ $# -ge 1 ] ;
then
    GIVEN_RISK_TAGS=$1; shift;
fi

#for getting data via fpga interface
FPGA="NO"

if [ $# -ge 1 ] ;
then
    FPGA=$1; shift;
fi


OUTPUTLOGDIR=/spare/local/logs/tradelogs ; 
PIDDIR=$OUTPUTLOGDIR/PID_TEXEC_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$PROGID"_"PIDfile.txt

cd $HOME

case $EXCH in
    cme|CME)
	;;
    eurex|EUREX)    
	;;
    tmx|TMX)    
	;;
    bmf|BMF)    
	;;
    liffe|LIFFE)
	;;
    hkex|HKEX)
	;;
    ose|OSE)
	;;
    rts|RTS)
	;;
    micex|MICEX)
	;;
    bmfeq|BMFEQ)
	;;

    *)    
	echo EXCH should be CME, EUREX, TMX , BMF , HKEX , OSE , RTS , MICEX or LIFFE
	;;
esac

LIVE_FLAG="LIVE"
if [ $FPGA = "FPGA" ]; then
   LIVE_FLAG="LIVE_FPGA"
fi
md5=`md5sum $TRADE_EXEC | awk '{print $1}'` ;

if [ -f $PIDFILE ] ;
then
    # print error & exit
    echo "Cannot start an instance of $TRADE_EXEC $STRATEGYDESCFILENAME $PROGID since $PIDFILE exists"
    exit;
else
    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi

    ulimit -c unlimited; 
    if [ $ONL = "ON" ] ;
    then
	export EF_LOG_VIA_IOCTL=1 ;
	export EF_NO_FAIL=0 ;
	export EF_UDP_RECV_MCAST_UL_ONLY=1 ;
	export EF_SPIN_USEC=1000000 ; 
	export EF_POLL_USEC=1000000 ; 
	export EF_SELECT_SPIN=1 ; 
	export EF_MULTICAST_LOOP_OFF=0 ;
	export EF_MAX_ENDPOINTS=1024 ;
	export EF_SHARE_WITH=-1;
	export EF_NAME=ORS_STACK ;

	onload $TRADE_EXEC $md5 $LIVE_FLAG $STRATEGYDESCFILENAME $PROGID TAGS $GIVEN_RISK_TAGS &
    elif [ $VMA = "ON" ] ;
    then
	export RDMAV_HUGEPAGES_SAFE=1 ;
	export VMA_TRACELEVEL=3 ;
	export VMA_LOG_DETAILS=3 ;
	export VMA_LOG_FILE=$HOME/vma_log_file.txt ;
	export VMA_QP_LOGIC=0 ;
	export VMA_RX_POLL=-1 ;
	export VMA_SELECT_POLL=-1 ;
	export VMA_APPLICATION_ID=TRD ;

	LD_PRELOAD=libvma.so $TRADE_EXEC $md5 $LIVE_FLAG $STRATEGYDESCFILENAME $PROGID TAGS $GIVEN_RISK_TAGS &
    else
	$TRADE_EXEC $md5 $LIVE_FLAG $STRATEGYDESCFILENAME $PROGID TAGS $GIVEN_RISK_TAGS &
    fi

    TRADE_EXEC_PID=$!
    echo $TRADE_EXEC_PID > $PIDFILE

    if [ $AFO = "AF" ] ;
    then
	echo $TRADE_EXEC_PID "tradeinit" >> /spare/local/files/affinity_pid_process.txt

	$AFFIN_EXEC ASSIGN $TRADE_EXEC_PID "tradeinit-$PROGID"
    fi
    #Notify risk client of the SACI-queryid-tag triplet (start in background so that it doesn't slow down prod process)
    /home/dvctrader/LiveExec/ModelScripts/update_query_risk_info.sh $PROGID $GIVEN_RISK_TAGS >> /spare/local/logs/tradelogs/risklog.$PROGID 2>&1 &
fi
