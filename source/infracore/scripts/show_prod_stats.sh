#!/bin/bash

USAGE="$0 YYYYMMDD ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

YYYYMMDD=$1; shift;

echo "EU_MORN_DAY";
START_UTC_HHMM=600;
END_UTC_HHMM=1200;
for name in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FGBX_0 FESX_0 FDAX_0 ;
do
    NUMTRADES=`~/infracore_install/bin/get_num_trades_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    VOLUME=`~/infracore_install/bin/get_volume_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    L1EVENTS=`~/infracore_install/bin/get_l1events_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    AVGL1SZ=`~/infracore_install/bin/get_avg_l1sz_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    AVGTRADESIZE=$(( $VOLUME / $NUMTRADES ));
    echo $name":" $L1EVENTS"," $NUMTRADES"," $VOLUME"," $AVGTRADESIZE"," $AVGL1SZ;
done

echo "US_MORN_DAY";
START_UTC_HHMM=1200;
END_UTC_HHMM=2000;
for name in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FGBX_0 FESX_0 FDAX_0 SXF_0 CGB_0 BAX_1 BAX_2 BAX_3 BAX_4 ; # BAX_5 BAX_6 BR_DOL_0 BR_IND_0 BR_WIN_0 BR_WDO_0 BR_DI_1 BR_DI_0 ;
#for name in BR_DOL_0 BR_IND_0 BR_WIN_0 BR_WDO_0 BR_DI_1 BR_DI_0 ;
do
    NUMTRADES=`~/infracore_install/bin/get_num_trades_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    VOLUME=`~/infracore_install/bin/get_volume_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    L1EVENTS=`~/infracore_install/bin/get_l1events_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    AVGL1SZ=`~/infracore_install/bin/get_avg_l1sz_on_day $name $YYYYMMDD $START_UTC_HHMM $END_UTC_HHMM | awk '{print $3}'`;
    AVGTRADESIZE=$(( $VOLUME / $NUMTRADES ));
    echo $name":" $L1EVENTS"," $NUMTRADES"," $VOLUME"," $AVGTRADESIZE"," $AVGL1SZ;
#    echo $name $L1EVENTS $NUMTRADES $VOLUME ;
done

