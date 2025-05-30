#!/bin/bash

USAGE="$0 EXCH ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRADE_EXEC=$HOME/LiveExec/bin/tradeinit

EXCH=$1; shift;

OUTPUTLOGDIR=/spare/local/logs/tradelogs ; 
PIDDIR=$OUTPUTLOGDIR/PID_TEXEC_DIR ;

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

for PIDFILE in $PIDDIR/$EXCH"_"*"_"PIDfile.txt
do
    if [ -f $PIDFILE ] ; then
	
	num_lines=`wc -l $PIDFILE | awk '{print $1}'` ;
	if [ $num_lines -gt 0 ] ; then

	    FILEOWNER=`ls -altr $PIDFILE | awk '{print $3}'`;
	    if [ "$FILEOWNER" = "$USER" ] ; then 
		
		TRADE_EXEC_PID=`tail -1 $PIDFILE`
		kill -2 $TRADE_EXEC_PID # SIGINT
		sleep 3;
		
		running_proc_string=`ps -p $TRADE_EXEC_PID -o comm=`;
		if [ $running_proc_string ] ; then 
#	    echo "patience ... "; 
		    sleep 4 ; 
		    
		    running_proc_string=`ps -p $TRADE_EXEC_PID -o comm=`;
		    if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
			echo "Sending SIGKILL $PIDFILE "; 
			kill -9 $TRADE_EXEC_PID
		    fi
		fi
		
		rm -f $PIDFILE
		
	    fi
	else
	# print error & exit
	    echo "problem processing" $PIDFILE ;
	fi    
    fi

done
