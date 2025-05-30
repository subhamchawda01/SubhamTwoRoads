#!/bin/bash



# ~/infracore_install/bindebug/all_volumes_on_day CME 20110912 ZNZ1_20110912
#/NAS1/data/CMELoggedData/CHI/2011/09/12/ZNZ1)--file

USAGE1="$0 EXCH YYYYMMDD "
EXAMP1="$0 CME 20110912"

if [ $# -ne 2 ] ; 
then 
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi
EXEC=$HOME/infracore_install/bin/all_volumes_on_day 


#COPY_LOG_EXEC=$HOME/LiveExec/scripts/ors_binary_log_backup.sh
#CLEAR_LOG_EXEC=$HOME/LiveExec/scripts/ors_binary_clear_log.sh

EXCH=$1;
#LOC=$2;
YYYYMMDD=$2;
FILE=$3;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi


case $EXCH in
    CME)
	LOC=CHI
	DIRLOC=/NAS1/data/CMELoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		$EXEC $LOC $files
	    done
	fi


	;;    
    EUREX)
	LOC=FR2
	DIRLOC=/NAS1/data/EUREXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		$EXEC $LOC $files
	    done
	fi
	;;

    TMX)
	LOC=TOR
	DIRLOC=/NAS1/data/TMXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		$EXEC $LOC  $files
	    done
	fi
	;;
    
    BMF)
	LOC=BMF
	DIRLOC=/NAS1/data/BMFLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		$EXEC $LOC $files
	    done
	fi
	;;
    
    *)
	echo "Not implemented for $EXCH";
esac

