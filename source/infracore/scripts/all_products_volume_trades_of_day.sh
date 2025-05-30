#!/bin/bash

USAGE1="$0 EXCH YYYYMMDD SORTBY[V/T]"
EXAMP1="$0 CME 20120427 V"

if [ $# -ne 3 ] ; 
then 
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

EXEC=$HOME/infracore_install/bin/get_all_products_volume_trades_on_day


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

TEMP_VOL_TRD_FILE=/tmp/temp_volume_trades_files.txt

>$TEMP_VOL_TRD_FILE ;

EXCH=$1;
YYYYMMDD=$2;
SORTBY=$3;

if [ $YYYYMMDD = "YESTERDAY" ] ;
then

    YYYYMMDD=`date +"%Y%m%d" -d "yesterday"`

fi


case $EXCH in
    CME)
	LOC=CHI
	DIRLOC=/NAS1/data/CMELoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
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
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
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
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi
	;;
    
    BMF)
	LOC=BRZ
	LOCD=NTP
	DIRLOC=/NAS1/data/NTPLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi

	;;

    BMFEQ)
        #BMF Equities
        LOC=BRZ
        LOCD=NTP  
	EXCH=BMF
        DIRLOC=/NAS1/data/BMFLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}

	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi

	;;

    HONGKONG)
	LOC=HK
	DIRLOC=/NAS1/data/HKEXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi
	;;

    OSE)
	LOC=TOK
	DIRLOC=/NAS1/data/OSELoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi
	;;

    MICEX)
	LOC=OTK
	DIRLOC=/NAS1/data/MICEXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi
	;;

    RTS)
	LOC=OTK
	DIRLOC=/NAS1/data/RTSLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi
	;;

    CHIX)
	LOC=FR2
	DIRLOC=/NAS1/data/CHIXLoggedData/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}
	if [ -d $DIRLOC ]; 
	    then 
	    for files in $DIRLOC/*
	    do 
		echo -e `echo "$files" | awk -F"/" '{print $NF}' | awk -F"_" '{print $1}'` '\t' `$EXEC $files $EXCH` >> $TEMP_VOL_TRD_FILE ;
	    done
	fi
	;;

    *)
	echo "Not implemented for $EXCH";
esac;

if [ $SORTBY = "T" ]
then 

  cat $TEMP_VOL_TRD_FILE | sort -nrk 3

else
 
  cat $TEMP_VOL_TRD_FILE | sort -nrk 2 

fi

rm -rf $TEMP_VOL_TRD_FILE
