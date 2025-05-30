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

TRADELOGDIR=/spare/local/logs/tradelogs
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

~/basetrade/scripts/plot_trades_pnl_nyc_2_all.sh $TRADELOGDIR/trades.$YYYYMMDD.$PREFIX??
