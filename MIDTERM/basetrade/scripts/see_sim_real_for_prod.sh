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
	UB_0)
	    MKT_MODEL=3;
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
	FOAT_0)
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
    EU_MORN_DAY_US_DAY)
	TIMEOFDAY=EUS;
	;;
esac

case $TIMEOFDAY in
    AS)
	case $SHORTCODE in
	    HHI_0)
		PREFIX=20;
		;;
	esac
	;;
    US|EUS)
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
	    FOAT_0)
		PREFIX=90;
		;;
	    FDAX_0)
		PREFIX=70;
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
	    BR_DOL_0)
		PREFIX=300;
		;;
            DI1F15)
                PREFIX=340;
                ;;
            DI1F18)
                PREFIX=360;
                ;;
            DI1F16)
                PREFIX=370;
                ;;
            DI1N14)
                PREFIX=380;
                ;;
            DI1F17)
                PREFIX=390;
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
	esac
	;;
esac

for TRADESFILENAME in $QUERYTRADEDIR/$SPLITDATEDIR/trades.$YYYYMMDD.$PREFIX?? ; 
do 
    if [ -e $TRADESFILENAME ] ; then
	PID=`echo $TRADESFILENAME | awk -F\. '{print $3}'`; 
	echo $PID ;
	~/basetrade/scripts/sim_real_and_plot.sh $PID $YYYYMMDD $MKT_MODEL; 
    fi
done
