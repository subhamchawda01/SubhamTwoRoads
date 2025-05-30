#!/bin/bash

YYYYMMDD=`date +%Y%m%d`;

USAGE="$0 TIMEOFDAY SHORTCODE [YYYYMMDD=TODAY]";
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
fi

PREFIX=1; #invalid

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
	    CGB_0)
		PREFIX=210;
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
	esac
	;;
    EU)
	;;
esac

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}

~/basetrade/scripts/plot_trades_pnl_nyc_2_all.sh /NAS1/logs/QueryTrades/$NEWDIR/trades.$YYYYMMDD.$PREFIX??
