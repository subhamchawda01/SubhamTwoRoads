#!/bin/bash

#YYYYMMDD=`date +%Y%m%d`;

USAGE="$0 TIMEOFDAY SHORTCODE START_DATE END_DATE";
if [ $# -lt 4 ] ; 
then 
    echo $USAGE;
    exit;
fi

TIMEOFDAY=$1; shift;
SHORTCODE=$1; shift;
STARTDATE=$1; shift; 
ENDDATE=$1; shift;

QUERYTRADESDIR=/NAS1/logs/QueryTrades
QUERYLOGDIR=/NAS1/logs/QueryLogs

PREFIX=120;

case $TIMEOFDAY in
    US)
	case $SHORTCODE in
	    FGBS_0)
		PREFIX=20;
		;;
	    FGBM_0)
		PREFIX=30;
		;;
	    FGBL_0)
		PREFIX=40;
		;;
	    FESX_0)
		PREFIX=60;
		;;
	    FDAX_0)
		;;
	    ZF_0)
		PREFIX=120;
		;;
	    ZN_0)
		PREFIX=130;
		;;
	    ZB_0)
		PREFIX=140;
		;;
	esac
	;;
    EU)
	case $SHORTCODE in
	    FGBS_0)
		PREFIX=3;
		;;
	    FGBM_0)
		PREFIX=1;
		;;
	    FGBL_0)
		PREFIX=4;
		;;
	    FESX_0)
		PREFIX=2;
		;;
	    FDAX_0)
		;;
	    ZF_0)
		PREFIX=6;
		;;
	    ZN_0)
		PREFIX=130;
		;;
	    ZB_0)
		PREFIX=140;
		;;
	esac
	;;
esac

YYYYMMDD=$STARTDATE;
NUMDAYS=0;
while [ $NUMDAYS -lt 90 ] 
do
    NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2};

    FULL_QUERY_TRADES_DIR=$QUERYTRADESDIR/$NEWDIR;
    FULL_QUERY_LOG_DIR=$QUERYLOGDIR/$NEWDIR;
    
    for TRADESFILENAME in $FULL_QUERY_TRADES_DIR/trades.$YYYYMMDD.$PREFIX?? ; 
    do 
	if [ -e $TRADESFILENAME ] ; 
	then 
	    REALID=`echo $TRADESFILENAME | awk -F\. '{print $3}'`; 
	    LOGFILENAME=$FULL_QUERY_LOG_DIR/log.$YYYYMMDD.$REALID.gz;
	    STRATFILE=`less $LOGFILENAME | head -1`; 
	    if [ -e ~/infracore/ModelScripts/get_pnl_stats.pl ] ; then
		FinalVolume=`~/infracore/ModelScripts/get_pnl_stats.pl $TRADESFILENAME | grep FinalVolume | awk '{print $2}'`; 
		FinalPNL=`~/infracore/ModelScripts/get_pnl_stats.pl $TRADESFILENAME | grep FinalPNL | awk '{print $2}'`; 
	    fi
	    echo $FinalPNL $FinalVolume $STRATFILE $YYYYMMDD ; 
	fi ; 
    done

    NUMDAYS=$(( $NUMDAYS + 1 ));
    YYYYMMDD=$(( $YYYYMMDD + 1 ));
    if [ $YYYYMMDD -gt $ENDDATE ] ; then
	NUMDAYS=1000;
    fi
done
