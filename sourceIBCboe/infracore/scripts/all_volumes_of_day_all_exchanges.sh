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
EXEC=$HOME/LiveExec/bin/all_volumes_on_day
if [ ! -e $EXEC ] ; then EXEC=$HOME/infracore_install/bin/all_volumes_on_day ; fi


YYYYMMDD=$1;
FILE=$3;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi
LOGDIR="/spare/local/VolumeBasedSymbol"
LOGFILE=$LOGDIR"/VOSymbol_"$YYYYMMDD".txt.orig"
#if directory doesnot exist make it
if [ ! -d $LOGDIR ]; then mkdir -p $LOGDIR
fi

#Add more exchange here later
for EXCH in ICE
#for EXCH in CME EUREX BMF TMX LIFFE HONGKONG OSE MICEX RTS CHIX CFE ICE
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

	LIFFE)
	    LOC=BSL
	    DIRLOC=/NAS1/data/LIFFELoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
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
	    DIRLOC=/NAS1/data/TMX_FSLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
		for files in $DIRLOC/*
		do
		     $EXEC $LOC  $files >> $LOGFILE
		done
	    fi
	    ;;

	BMF)
            #NTP data
	    LOC=BRZ
	    LOCD=NTP
	    DIRLOC=/NAS1/data/NTPLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
		for files in $DIRLOC/*
		do
		     $EXEC $LOCD $files >> $LOGFILE
		done
	    fi

            #Equities
	    DIRLOC=/NAS1/data/BMFLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
		for files in $DIRLOC/*
		do
		     $EXEC $LOCD $files >> $LOGFILE
		done
	    fi
	    ;;

	HONGKONG)
	    LOC=HK
            DATASTRUCT=HKEX
	    DIRLOC=/NAS1/data/HKEXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
		for files in $DIRLOC/*
		do
		     $EXEC $LOC $files >> $LOGFILE
		done
	    fi
	    ;;

	OSE)
	    LOC=TOK
        DATASTRUCT=TOK
	    DIRLOC=/NAS1/data/OSENewPriceFeedLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
		for files in $DIRLOC/*
		do
		     $EXEC $LOC $files >> $LOGFILE
		done
	    fi
	    ;;

	MICEX)
	    LOC=MOS
	    LOCD=MICEX
	    DIRLOC=/NAS1/data/MICEXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
	        for files in $DIRLOC/*
		do
		    $EXEC $LOCD $files >> $LOGFILE
		done
	    fi
	    ;;

	RTS)
	    LOC=MOS
	    LOCD=RTS
	    DIRLOC=/NAS1/data/RTSLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
	        for files in $DIRLOC/*
		do
		    $EXEC $LOCD $files >> $LOGFILE
		done
	    fi
	    ;;

	CHIX)
	    LOC=FR2
	    LOCD=CHIX
	    DIRLOC=/NAS1/data/CHIXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
	        for files in $DIRLOC/*
		do
		    $EXEC $LOCD $files >> $LOGFILE
		done
	    fi
	    ;;


	CFE)
	    LOC=CFE
	    LOCD=CFE
	    DIRLOC=/NAS1/data/CSMLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
	        for files in $DIRLOC/*
		do
		    $EXEC $LOCD $files >> $LOGFILE
		done
	    fi
	    ;;

	ICE)
	    LOC=BSL
	    LOCD=BSL
	    DIRLOC=/NAS1/data/ICELoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	    if [ -d $DIRLOC ];
	    then
	        for files in $DIRLOC/*
		do
		    $EXEC $LOCD $files >> $LOGFILE
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
