#!/bin/bash
vol1=`grep "$1 " $HOME/basetrade/hftrap/SpreadTrading/volume_data | awk '{print $2}'`
vol2=`grep "$2 " $HOME/basetrade/hftrap/SpreadTrading/volume_data | awk '{print $2}'`

if [ $vol1 -le $vol2 ]; then
	passLeg='1';
else
	passLeg='0';
fi

nm1=`echo $1 | tr '[~]' '&'`
nm2=`echo $2 | tr '[~]' '&'`

echo "NAV                             10000000
RISK_FACTOR                     0.25            #15%
INSTRUMENT_1			"$nm1"
INSTRUMENT_2                    "$nm2"
HEDGE_FACTOR 1.0
MARGIN_1                        0.15            #15%
MARGIN_2                        0.15            #15%
SMA_LENGTH 150          #in mins
SPREAD_HIST_LENGTH 150          #in mins
TRADE_COOLOFF_INTERVAL          100             #in secs
MAX_UNIT_RATIO 1        #no multileg entry
ENTRY_SPREAD_THRESHOLD 2.5              #zscore of 2
INCREMENTAL_ENTRY_THRESHOLD 0.01                #irrelevant if MAX_UNIT_RATIO is 1 - additional 0.5 stdev deviation
ENTRY_EXIT_DIFFERENTIAL 2.5      
MINIMUM_ABSOLUTE_THRESHOLD 0.025
STOP_LOSS                       20000000                #10%
STOP_GAIN                       20000000                #10%
ZSCORE_VEC_LEN 2500
SPREAD_COMP_MODE 1
ASSET_COMP_MODE 0
RETURN_COMP_DURATION 60
IS_INST1_PASS "$passLeg"
PASS_ORD_MOD_TIME_THRESHOLD 5000
AGG_ORD_MOD_TIME_THRESHOLD 1000
PASS_ORD_MOD_STD_THRESHOLD 0
AGG_ORD_MOD_STD_THRESHOLD 0
TARGET_VOL 0.2  #15%
PARAM_ID	"$3;
