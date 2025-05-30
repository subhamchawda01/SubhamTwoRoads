#!/bin/bash

USAGE="$0 EXCH PROGID STRATEGYDESCFILENAME ";
if [ $# -ne 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRADE_EXEC=$HOME/LiveExec/bin/tradeinit

EXCH=$1; shift;
PROGID=$1; shift;
STRATEGYDESCFILENAME=$1; shift;

OUTPUTLOGDIR=/spare/local/logs/tradelogs ; 
PIDDIR=$OUTPUTLOGDIR/PID_TEXEC_DIR ;

PIDFILE=$PIDDIR/$EXCH"_"$PROGID"_"PIDfile.txt

cd $HOME

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

case $EXCH in
    cme|CME)
	;;
    eurex|EUREX)    
	;;
    tmx|TMX)    
	;;
    bmf|BMF)    
	;;
    *)    
	echo EXCH should be CME, EUREX, TMX or BMF
	;;
esac

if [ -f $PIDFILE ] ;
then
    # print error & exit
    echo "Cannot start an instance of $TRADE_EXEC $STRATEGYDESCFILENAME $PROGID since $PIDFILE exists"
    exit;
else
    if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi
    if [ ! -d $OUTPUTLOGDIR ] ; then mkdir -p $OUTPUTLOGDIR ; fi

    $TRADE_EXEC LIVE $STRATEGYDESCFILENAME $PROGID &
    TRADE_EXEC_PID=$!
    echo $TRADE_EXEC_PID > $PIDFILE
fi
