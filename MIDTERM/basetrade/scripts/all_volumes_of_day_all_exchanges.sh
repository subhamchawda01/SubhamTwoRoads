#!/bin/bash

# ~/infracore_install/bindebug/all_volumes_on_day CME 20110912 ZNZ1_20110912
#/NAS1/data/CMELoggedData/CHI/2011/09/12/ZNZ1)--file

USAGE1="$0 YYYYMMDD "
EXAMP1="$0 20110912"

if [ $# -ne 1 ] ; 
then 
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi
EXEC=$HOME/infracore_install/bin/all_volumes_on_day 

YYYYMMDD=$1;
FILE=$3;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi
LOGDIR="/spare/local/VolumeBasedSymbol/" 
LOGFILE=$LOGDIR"/VOSymbol_"$YYYYMMDD".txt.orig"
#if directory doesnot exist make it
if [ ! -d $LOGDIR ]; then mkdir -p $LOGDIR 
fi

#Add more exchange here later
for EXCH in CME EUREX BMF TMX
do 

    case $EXCH in
	CME)
	    LOC=CHI
	    DIRLOC=/NAS1/data/CMELoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ]; 
	    then 
		for files in $DIRLOC/*
		do 
		     $EXEC $LOC $files >> $LOGFILE
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
		    $EXEC $LOC $files >> $LOGFILE
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
		     $EXEC $LOC  $files >> $LOGFILE
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
		     $EXEC $LOC $files >> $LOGFILE
		done
	    fi
	    ;;
	
	*)
	    echo "Not implemented for $EXCH";
    esac
done
#finally we just need the symbol
#/NAS1/data/CMELoggedData/CHI/2011/10/11/6EZ1_20111011.gz        275018
# will be converted to 6EZ1 275018
cat $LOGFILE | sort -n -k2 | awk '{split($0, a, "/"); split( a[9], b, "_"); print b[1], $2}' > $LOGDIR"/VOSymbol_"$YYYYMMDD".basecodes.txt"
