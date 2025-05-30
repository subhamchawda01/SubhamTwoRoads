#!/bin/bash


USAGE="$0 TIMEOFDAY SHORTCODE [YYYYMMDD=TODAY] [MKT_MODEL=-1]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

TIMEOFDAY=$1; shift;
SHORTCODE=$1; shift;

if [ $# -gt 0 ] ; 
then 
    YYYYMMDD=$1; shift; 
else
    YYYYMMDD=`date +%Y%m%d`;
fi

if [ $# -gt 0 ] ; 
then 
    MKT_MODEL=$1; shift; 
else
    MKT_MODEL=-1;

    case $SHORTCODE
	in
	ZT_0)
	    MKT_MODEL=1;
	    ;;
	ZF_0)
	    MKT_MODEL=1;
	    ;;
	ZN_0)
	    MKT_MODEL=1;
	    ;;
	ZB_0)
	    MKT_MODEL=2;
	    ;;
	FGBS_0)
	    MKT_MODEL=3;
	    ;;
	FGBM_0)
	    MKT_MODEL=3;
	    ;;
	FGBL_0)
	    MKT_MODEL=3;
	    ;;
	FESX_0)
	    MKT_MODEL=3;
	    ;;
	CGB_0)
	    MKT_MODEL=2;
	    ;;
	*)
	    MKT_MODEL=2;
	    ;;
    esac
    
fi

SPLITDATEDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}

QUERYTRADEDIR=/NAS1/logs/QueryTrades;
PREFIX=1;#invlaid

case $TIMEOFDAY in
    US_MORN_DAY)
	TIMEOFDAY=US;
	;;
    EU_MORN_DAY)
	TIMEOFDAY=EU;
	;;
esac

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
	    SXF_0)
		PREFIX=200;
		;;
	    CGB_0)
		PREFIX=210;
		;;
	    BAX_0)
		PREFIX=220;
		;;
	    BAX_1)
		PREFIX=230;
		;;
	    BAX_2)
		PREFIX=240;
		;;
	    BAX_3)
		PREFIX=250;
		;;
	    BAX_4)
		PREFIX=260;
		;;
	    BAX_5)
		PREFIX=270;
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

for TRADESFILENAME in $QUERYTRADEDIR/$SPLITDATEDIR/trades.$YYYYMMDD.$PREFIX?? ; 
do 
    if [ -e $TRADESFILENAME ] ; then
	PID=`echo $TRADESFILENAME | awk -F\. '{print $3}'`; 
	echo $PID ;
	~/basetrade/scripts/sim_real_and_stats.sh $PID $YYYYMMDD $MKT_MODEL; 
    fi
done
