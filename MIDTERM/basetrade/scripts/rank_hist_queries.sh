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

PREFIX=0; #invalid

case $TIMEOFDAY in
    EU_MORN_DAY)
	TIMEOFDAY=EU;
	;;
    US_MORN_DAY)
	TIMEOFDAY=US;
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
	    FGBX_0)
		PREFIX=50;
		;;
	    FESX_0)
		PREFIX=60;
		;;
	    FOAT_0)
		PREFIX=90;
		;;
            FBTP_0)
                PREFIX=80;
                ;;
            FBTS_0)
                PREFIX=10;
                ;;
	    FDAX_0)
		PREFIX=70;
		;;
	    FVS_0)
		PREFIX=670;
		;;
	    FVS_1)
		PREFIX=680;
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
	    HHI_0)
		PREFIX=1000;
		;;
	    HSI_0)
		PREFIX=1100;
		;;
	    MHI_0)
		PREFIX=1300;
		;;
	    VX_0)
		PREFIX=620;
		;;
	    VX_1)
		PREFIX=630;
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
	    FOAT_0)
		PREFIX=7;
		;;
	    FBTP_0)
		PREFIX=8;
		;;
	    FDAX_0)
		;;
	    FVS_0)
		PREFIX=670;
		;;
	    FVS_1)
		PREFIX=680;
		;;
	    ZF_0)
		PREFIX=6;
		;;
	    ZN_0)
		PREFIX=9;
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
	    NK_0)
		PREFIX=2100;
		;;
	    NKM_0)
		PREFIX=2200;
		;;
	    HHI_0)
		PREFIX=1000;
		;;
	    HSI_0)
		PREFIX=1100;
		;;
	    MHI_0)
		PREFIX=1300;
		;;
	esac
	;;
esac

YYYYMMDD=$STARTDATE;
NUMDAYS=0;

hget(){
    eval echo '${queries_'$1'}'
}

while [ $NUMDAYS -lt 620 ] 
do
    NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2};

    FULL_QUERY_TRADES_DIR=$QUERYTRADESDIR/$NEWDIR;
    FULL_QUERY_LOG_DIR=$QUERYLOGDIR/$NEWDIR;
    
    if ls $FULL_QUERY_LOG_DIR/log.$YYYYMMDD.$PREFIX??.gz &> /dev/null; then 
	qs=$(zgrep STRATEGYLINE $FULL_QUERY_LOG_DIR/log.$YYYYMMDD.$PREFIX??.gz | awk '{if ($9) print $8":"$9; else print $8":NIL"}' | sort -u)
	
	for q in $qs;
	do 
	    query=${q##*:}; id=${q%%:*}
#	    echo $q "Query: $query" "id: $id"
	    if [[ "$query" = "NIL" ]]; then 
		query=`zcat $FULL_QUERY_LOG_DIR/log.$YYYYMMDD.$PREFIX??.gz | head -n 1 | xargs basename | sed 's/_[0-9]*$//g'`;
	    fi;
	    eval queries_$id=${query}
	done

	for TRADESFILENAME in $FULL_QUERY_TRADES_DIR/trades.$YYYYMMDD.$PREFIX?? ; 
	do
	    if [ -e $TRADESFILENAME ] ; 
	    then 
		REALID=`echo $TRADESFILENAME | awk -F\. '{print $3}'`; 
		LOGFILENAME=$FULL_QUERY_LOG_DIR/log.$YYYYMMDD.$REALID.gz;
            ## -f can't be used without terminal 
            ## either use --line-buffered or zcat
		
	    #STRATFILE=`zcat $LOGFILENAME | head -1`;fi # Not required.
		FinalVolume=`~/basetrade/ModelScripts/get_pnl_stats.pl $TRADESFILENAME | grep FinalVolume | awk '{print $2}'`; 
		FinalPNL=`~/basetrade/ModelScripts/get_pnl_stats.pl $TRADESFILENAME | grep FinalPNL | awk '{print $2}'`; 
	    #echo "~/basetrade/ModelScripts/get_pnl_stats_2.pl $TRADESFILENAME H";
		pnl_stats=`~/basetrade/ModelScripts/get_pnl_stats_2.pl $TRADESFILENAME H | cut -d' ' -f1,2,3 | sed 's/ /:/g'`;
		for pnl in $pnl_stats; do 
		    id=${pnl%%:*}; p=${pnl#*:}; p=${p//:/ }; 
		    #eval queries_$id=$p" "`hget($id)`" $YYYYMMDD"; 
		    echo $p" "`hget $id`"_$id $YYYYMMDD"
	    done
		#echo $pnl_stats $STRATFILE $YYYYMMDD;
	    fi;
	done
    fi;
    NUMDAYS=$(( $NUMDAYS + 1 ));
    
    MMDD=${YYYYMMDD:4:4};
    if [ $MMDD -eq "1231" ] ; then
	YYYYMMDD=$(( $YYYYMMDD + 8870 )); # 20120101 - 20111231
    else
	DD=${YYYYMMDD:6:2};
	if [ $DD -eq "31" ]; then
	    YYYYMMDD=$(( $YYYYMMDD + 70 )); # 20120201 - 20120131
	else
	    YYYYMMDD=$(( $YYYYMMDD + 1 ));
	fi
    fi
    
    if [ $YYYYMMDD -gt $ENDDATE ] ; then
	NUMDAYS=1000;
    fi
done
