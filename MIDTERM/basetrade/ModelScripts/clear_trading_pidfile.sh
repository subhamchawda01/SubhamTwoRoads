#!/bin/bash

USAGE="$0 EXCH PROGID ";
if [ $# -ne 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

EXCH=$1; shift;
PROGID=$1; shift;

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
    *)    
	echo EXCH should be CME, EUREX, TMX or BMF
	;;
esac

if [ -f $PIDFILE ] ;
then
    rm -f $PIDFILE ;
else
    echo "PIDFILE $PIDFILE does not exist";
fi
