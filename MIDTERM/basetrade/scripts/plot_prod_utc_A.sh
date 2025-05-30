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

PREFIX=1; # invalid

case $TIMEOFDAY in
    AS)
	case $SHORTCODE in
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
            YFEBM_0)
                PREFIX=410;
                ;;
            YFEBM_1)
                PREFIX=420;
                ;;
            YFEBM_2)
                PREFIX=430;
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
		;;
	    ZF_0)
		PREFIX=6;
		;;
	    ZN_0)
		PREFIX=9;
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

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}

~/basetrade/scripts/plot_trades_pnl_utc_2_A.sh /NAS1/logs/QueryTrades/$NEWDIR/trades.$YYYYMMDD.$PREFIX??
