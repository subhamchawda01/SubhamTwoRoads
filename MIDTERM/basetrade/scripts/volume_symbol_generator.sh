#!/bin/bash
#MUST BE RUN ONLY AFTER the MDS LoggedData has been created on NY4 machine for
#each instrument e.g apps/data/CMELoggedData/CHI/2011/08/12/CLX1_20110812

USAGE1="$0 YYYYMMDD CMD[START/STOP]";
USAGE2="$0 YYYYMMDD CMD[COPYDATA/COPYALL]";

if [ $# -ne 2 ] ; 
then 
    echo $USAGE1;
    echo $USAGE2;
    exit;
fi

VOLSYM_EXEC=$HOME/LiveExec/bin/symbol_driver_main
COPY_VOLSYM_EXEC=$HOME/LiveExec/bin/copy_volsym.pl

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

CMD=$2;
PIDDIR=/spare/local/VolumeBasedSymbol/PID_VOLSYM_DIR ;

mkdir -p $PIDDIR
PIDFILE=$PIDDIR/VolumeSym_$YYYYMMDD"_PIDfile.txt";

if [ $PWD != $HOME ]; then cd $HOME; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ];
	then 
	    	    # print error & exit
	    echo "Cannot start an instance of $VOLSYM_EXEC $YYYYMMDD since $PIDFILE exists"
	else
	    $VOLSYM_EXEC $YYYYMMDD &
	    VOLSYMLOGGERPID=$!
	    echo $VOLSYMLOGGERPID > $PIDFILE
	fi
	;;
    stop|STOP)
	if [ -f $PIDFILE ];
	then 
	 # Stop the process
	    VOLSYMLOGGERPID=`tail -1 $PIDFILE`
	    kill -2 $VOLSYMLOGGERPID
	    sleep 1;
	    running_proc_string=`ps -p $VOLSYMLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then 
		echo "patience..."
		sleep 4;
		running_proc_string=`ps -p $VOLSYMLOGGERPID -o comm=`
		if [ $running_proc_string ] ; then 
		    echo "sending SIGKILL";
		    kiil -9 $VOLSYMLOGGERPID
		fi
	    fi
	    rm -f $PIDFILE 
	    
	else
	    echo "Cannot stop an instance of $VOLSYM_EXEC $YYYYMMDD since $VOLSYMLOGGERPID exists"
	fi
	;;

    copydata|COPYDATA)
	ARG="/spare/local/VolumeBasedSymbol/VOSymbol_"$YYYYMMDD".txt"
	echo "copying to all machines  $ARG"
	echo $COPY_VOLSYM_EXEC
	`perl -w "$COPY_VOLSYM_EXEC" $ARG`
	;;

    copyall|COPYALL)
	#ARCH_NAME=VOS.tar
	#DIR=/spare/local/VolumeBasedSymbol
	#echo $0" : Archiving logs into "$ARCH_NAME.gz
	#cd $DIR
	#tar -cf $ARCH_NAME VOSymbol_* #all files with this pattern
	#echo $? > /tmp/VOS.code
	#gzip $ARCH_NAME
	#ARG=$DIR/$ARCH_NAME.gz
	#echo "copying to all machines  $ARG"
	#COPY_VOLSYM_EXEC=$HOME/infracore_install/scripts/copy_volsym.pl
	#echo $COPY_VOLSYM_EXEC
	echo $COPY_VOLSYM_EXEC
	DIR=/spare/local/VolumeBasedSymbol	
	for i in $(ls $DIR); do
	    item=$DIR/$i
#	    echo $item
	    if [[ $item =~ VOSymbol_* ]]
		then 
		echo $item
		`perl -w "$COPY_VOLSYM_EXEC" $item`
	    fi
#	`perl -w "$COPY_VOLSYM_EXEC" $ARG`
	done
	;;

esac

