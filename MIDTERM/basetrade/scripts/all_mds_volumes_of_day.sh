#!/bin/bash



# ~/infracore_install/bindebug/all_volumes_on_day CME 20110912 

USAGE1="$0 EXCH YYYYMMDD "
EXAMP1="$0 CME 20110912"

if [ $# -ne 2 ] ; 
then 
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi
EXEC=$HOME/infracore_install/bin/all_volumes_on_day 


EXCH=$1;
YYYYMMDD=$2;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi


case $EXCH in
    CME)
	LOC=CHI
	DIRLOC=/spare/local/MDSlogs/$EXCH
	if [ -d $DIRLOC ]; 
	    then 
	    for files in `ls $DIRLOC | grep $YYYYMMDD` 
	    do 
		$EXEC $LOC $DIRLOC/$files
	    done
	fi


	;;    
    EUREX)
	LOC=FR2
	DIRLOC=/spare/local/MDSlogs/$EXCH
	if [ -d $DIRLOC ]; 
	    then 
	    for files in `ls $DIRLOC | grep $YYYYMMDD` 
	    do 
		$EXEC $LOC $DIRLOC/$files
	    done
	fi
	;;

    TMX)
	LOC=TOR
	DIRLOC=/spare/local/MDSlogs/$EXCH
	if [ -d $DIRLOC ]; 
	    then 
	    for files in `ls $DIRLOC | grep $YYYYMMDD` 
	    do 
		$EXEC $LOC $DIRLOC/$files
	    done
	fi
	;;
    
    NTP)
	LOC=BRZ
	DIRLOC=/spare/local/MDSlogs/$EXCH
	if [ -d $DIRLOC ]; 
	    then 
	    for files in `ls $DIRLOC | grep $YYYYMMDD` 
	    do 
		$EXEC $LOC $DIRLOC/$files
	    done
	fi
	;;
    
    *)
	echo "Not implemented for $EXCH";
esac

