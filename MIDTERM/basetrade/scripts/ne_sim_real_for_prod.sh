#!/bin/bash

YYYYMMDD=`date +%Y%m%d`;

USAGE="$0 TIMEOFDAY SHORTCODE [YYYYMMDD=TODAY] [REMOVE_STRAT_FILES=1]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

TIMEOFDAY=$1; shift;
SHORTCODE=$1; shift;

TRADEDATE=$YYYYMMDD;
if [ $# -ge 1 ] ; 
then 
    TRADEDATE=$1; shift;
fi

REMOVE_STRAT_FILES="";
if [ $# -ge 1 ] ; 
then 
    REMOVE_STRAT_FILES=$1; shift;
fi

SPLITDATEDIR=${TRADEDATE:0:4}/${TRADEDATE:4:2}/${TRADEDATE:6:2}

QUERYTRADEDIR=/NAS1/logs/QueryTrades;
PREFIX=1;#invlaid

case $TIMEOFDAY in
    US_MORN_DAY)
	TIMEOFDAY=US;
	;;
    EU_MORN_DAY)
	TIMEOFDAY=EU;
	;;
    AS_MORN)
	TIMEOFDAY=AS;
	;;
    AS_DAY)
	TIMEOFDAY=AS;
	;;
    AS_MORN_DAY)
	TIMEOFDAY=AS;
	;;
esac

case $SHORTCODE in
    SXF_0)
	PREFIX=200;
	;;
    FGBX_0)
	PREFIX=50;
	;;
    HHI_0)
	PREFIX=1000;
	;;
    HSI_0)
	PREFIX=1100;
	;;
    MCH_0)
	PREFIX=1200;
	;;
    MHI_0)
	PREFIX=1300;
	;;
    CGB_0)
	PREFIX=210;
	;;
esac

case $TIMEOFDAY in
    AS)
	case $SHORTCODE in
	    NK_0)
		PREFIX=2100;
		;;
	    NKM_0)
		PREFIX=2200;
		;;
	    NKM_1)
		PREFIX=2300;
		;;
	esac
	;;
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
	    FGBX_0)
		PREFIX=50;
		;;
	    FESX_0)
		PREFIX=60;
		;;
	    FOAT_0)
		PREFIX=90;
		;;
	    FDAX_0)
		PREFIX=70;
		;;
	    FBTP_0)
		PREFIX=80;
		;;
            FBTS_0)
                PREFIX=10;
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
	    UB_0)
		PREFIX=150;
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
	    BR_DOL_0)
		PREFIX=300;
		;;
            DI1F15)
                PREFIX=350;
                ;;
            DI1F16)
                PREFIX=390;
                ;;
            DI1F17)
                PREFIX=380;
                ;;
            DI1F18)
                PREFIX=370;
                ;;
            DI1N14)
                PREFIX=308;
                ;;
	    BR_IND_0)
		PREFIX=310;
		;;
	    BR_WIN_0)
		PREFIX=320;
		;;
	    JFFCE_0)
		PREFIX=400;
		;;
	    KFFTI_0)
		PREFIX=500;
		;;
	    LFR_0)
		PREFIX=600;
		;;
	    LFZ_0)
		PREFIX=700;
		;;
            YFEBM_0)
                PREFIX=410;
                ;;
            YFEBM_1)
                PREFIX=420;
                ;;
            YFEBM_2)
                PREFIX=430;
                ;;
            XFC_0)
                PREFIX=510;
                ;;
	    LFL_0)
		PREFIX=800;
		;;
	    LFL_1)
		PREFIX=801;
		;;
	    LFL_2)
		PREFIX=802;
		;;
	    LFL_3)
		PREFIX=803;
		;;
	    LFL_4)
		PREFIX=804;
		;;
	    LFL_5)
		PREFIX=805;
		;;
	    LFL_6)
		PREFIX=806;
		;;
	    LFI_0)
		PREFIX=900;
		;;
	    LFI_1)
		PREFIX=901;
		;;
	    LFI_2)
		PREFIX=902;
		;;
	    LFI_3)
		PREFIX=903;
		;;
	    LFI_4)
		PREFIX=904;
		;;
	    LFI_5)
		PREFIX=905;
		;;
	    LFI_6)
		PREFIX=906;
		;;
	    NK_0)
		PREFIX=2100;
		;;
	    NKM_0)
		PREFIX=2200;
		;;
	    NKM_1)
		PREFIX=2300;
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
		PREFIX=70;
		;;
	    FBTP_0)
		PREFIX=8;
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
	    JFFCE_0)
		PREFIX=10;
		;;
	    KFFTI_0)
		PREFIX=11;
		;;
	    LFR_0)
		PREFIX=12;
		;;
	    LFZ_0)
		PREFIX=13;
		;;
            YFEBM_0)
                PREFIX=410;
                ;;
            YFEBM_1)
                PREFIX=420;
                ;;
            YFEBM_2)
                PREFIX=430;
                ;;
            XFC_0)
                PREFIX=510;
                ;;
	    LFL_0)
		PREFIX=800;
		;;
	    LFL_1)
		PREFIX=801;
		;;
	    LFL_2)
		PREFIX=802;
		;;
	    LFL_3)
		PREFIX=803;
		;;
	    LFL_4)
		PREFIX=804;
		;;
	    LFL_5)
		PREFIX=805;
		;;
	    LFL_6)
		PREFIX=806;
		;;
	    LFI_0)
		PREFIX=900;
		;;
	    LFI_1)
		PREFIX=901;
		;;
	    LFI_2)
		PREFIX=902;
		;;
	    LFI_3)
		PREFIX=903;
		;;
	    LFI_4)
		PREFIX=904;
		;;
	    LFI_5)
		PREFIX=905;
		;;
	    LFI_6)
		PREFIX=906;
		;;
	    NK_0)
		PREFIX=2100;
		;;
	    NKM_0)
		PREFIX=2200;
		;;
	    NKM_1)
		PREFIX=2300;
		;;
	esac
	;;
esac

DEBUG=1
if [ $DEBUG -gt "0" ] ; then 
echo $QUERYTRADEDIR/$SPLITDATEDIR/trades.$TRADEDATE.$PREFIX?? ;
fi
for TRADESFILENAME in $QUERYTRADEDIR/$SPLITDATEDIR/trades.$TRADEDATE.$PREFIX?? ; 
do 
    if [ -e $TRADESFILENAME ] ; then
	PID=`echo $TRADESFILENAME | awk -F\. '{print $3}'`; 
	echo REALID $PID ;

	if [ $DEBUG -gt "0" ] ; then 
	    echo $HOME/basetrade/scripts/run_accurate_no_end_sim_real_stratid.pl $TRADEDATE $PID $REMOVE_STRAT_FILES;
	fi
	$HOME/basetrade/scripts/run_accurate_no_end_sim_real_stratid.pl $TRADEDATE $PID $REMOVE_STRAT_FILES;
    fi
done
