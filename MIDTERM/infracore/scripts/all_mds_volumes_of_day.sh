#!/bin/bash



# all_volumes_on_day CME 20110912

USAGE1="$0 EXCH YYYYMMDD "
EXAMP1="$0 CME 20110912"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi
EXEC=$HOME/LiveExec/bin/all_volumes_on_day
if [ ! -e $EXEC ] ; then EXEC=$HOME/infracore_install/bin/all_volumes_on_day ; fi

if [ -e $HOME/.gcc_profile ] ;
then
    source $HOME/.gcc_profile
else
    export NEW_GCC_LIB=/usr/local/lib
    export NEW_GCC_LIB64=/usr/local/lib64
    export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH
fi

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

    LIFFE)
	LOC=BSL
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
	LOC=NTP
	DIRLOC=/spare/local/MDSlogs/$EXCH
	if [ -d $DIRLOC ];
	    then
	    for files in `ls $DIRLOC | grep $YYYYMMDD`
	    do
		$EXEC $LOC $DIRLOC/$files
	    done
	fi
	;;

    BMFEQ)
	LOC=BMF
        DATASTRUCT=NTP
	DIRLOC=/spare/local/MDSlogs/$LOC
	if [ -d $DIRLOC ];
	    then
	    for files in `ls $DIRLOC | grep $YYYYMMDD`
	    do
		$EXEC $DATASTRUCT $DIRLOC/$files
	    done
	fi
	;;

    HONGKONG)
	LOC=HKEX
	ELOC=HK
        DATASTRUCT=HKEX
	DIRLOC=/spare/local/MDSlogs/$LOC
	if [ -d $DIRLOC ];
	    then
	    for files in `ls $DIRLOC | grep $YYYYMMDD`
	    do
		$EXEC $ELOC $DIRLOC/$files
	    done
	fi
	;;

    OSE)
	LOC=TOK
        DATASTRUCT=OSEPriceFeed
	DIRLOC=/spare/local/MDSlogs/$DATASTRUCT
	if [ -d $DIRLOC ];
	    then
	    for files in `ls $DIRLOC | grep $YYYYMMDD`
	    do
		$EXEC $LOC $DIRLOC/$files
	    done
	fi
	;;

    MICEX)
        LOC=OTK
        LOCD=MICEX
        DIRLOC=/spare/local/MDSlogs/$EXCH
        if [ -d $DIRLOC ];
        then
            for files in `ls $DIRLOC | grep $YYYYMMDD`
            do
                $EXEC $LOCD $DIRLOC/$files
            done
        fi
        ;;

    RTS)
        LOC=OTK
        LOCD=RTS
        DIRLOC=/spare/local/MDSlogs/$EXCH
        if [ -d $DIRLOC ];
        then
            for files in `ls $DIRLOC | grep $YYYYMMDD`
            do
                $EXEC $LOCD $DIRLOC/$files
            done
        fi
        ;;

    CHIX)
        LOC=FR2
        LOCD=CHIX
        DIRLOC=/spare/local/MDSlogs/$EXCH
        if [ -d $DIRLOC ];
        then
            for files in `ls $DIRLOC | grep $YYYYMMDD`
            do
                $EXEC $LOCD $DIRLOC/$files
            done
        fi
        ;;


    *)
	echo "Not implemented for $EXCH";
esac

