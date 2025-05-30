#!/bin/bash

TRD_EXEC=$HOME/LiveExec/bin/trade_monitor

CMD=$1; shift;

PIDDIR=/spare/local/ORSlogs/PID_ORS_DIR ;
PIDFILE=$PIDDIR/"TRADE_MON_"$INSTRUMENT"_"PIDfile.txt

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
	    # print error & exit
	    echo "Cannot start an instance of $TRD_EXEC $INSTRUMENT since $PIDFILE exists"
	else
	    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi

	    $TRD_EXEC ZN_0 ZF_0 ZB_0 ZT_0 FGBL_0 FGBM_0 FGBS_0 FESX_0 FDAX_0 --log-stats &
	    ORSPID=$!
	    echo $ORSPID > $PIDFILE
	fi    
	;;

    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
	    ORSPID=`tail -1 $PIDFILE`
	    kill -2 $ORSPID

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $TRD_EXEC $INSTRUMENT since $PIDFILE does not exist"
	fi    

	;;

    *)
	echo CMD $CMD not expected
	;;
esac

