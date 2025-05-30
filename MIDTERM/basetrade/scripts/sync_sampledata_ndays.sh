#!/bin/bash

if [ $# -lt 1 ]; then echo "$0 DATE [SHC/ALL]"; exit; fi

YYYYMMDD=$1;
SHCLIST=${2:-ALL}
NUMDAYS=60;

if [ $YYYYMMDD = "TODAY" ] ; then YYYYMMDD=$(date "+%Y%m%d"); fi

TEMP_FEAT_DIR="/spare/local/temp/Features";
FEAT_DIR="/spare/local/Features";

if [[ $SHCLIST == "ALL" ]]; then
    SHCLIST=`ls -1 $FEAT_DIR 2>/dev/null`;
fi

for SHC in $SHCLIST ; do 
    if [ -d $FEAT_DIR/$SHC ]; then
        echo $SHC;
        if [ ! -d $TEMP_FEAT_DIR/$SHC ] ; then mkdir -p $TEMP_FEAT_DIR/$SHC ; chown :infra $TEMP_FEAT_DIR/$SHC; fi;
        DTS=`$HOME/basetrade/scripts/get_list_of_dates_for_shortcode.pl $SHC $YYYYMMDD $NUMDAYS`;
        for DT in `ls -1 $TEMP_FEAT_DIR/$SHC`; do
            if ! [[ " $DTS " =~ " $DT " ]] || ! [ -d $FEAT_DIR/$SHC/$DT ] ; then
            rm -rf $TEMP_FEAT_DIR/$SHC/$DT;
            fi;
        done;
        for DT in $DTS AvgSamples; do
            if [ -d $FEAT_DIR/$SHC/$DT ] ; then
            rsync -raq $FEAT_DIR/$SHC/$DT $TEMP_FEAT_DIR/$SHC/;
            fi;
        done;
        $HOME/basetrade_install/scripts/sync_dir_to_trade_machines.pl $TEMP_FEAT_DIR/$SHC $FEAT_DIR/$SHC dvctrader 1 "--exclude 'NSE_*'" -1 ind;
        $HOME/basetrade_install/scripts/sync_dir_to_trade_machines.pl $TEMP_FEAT_DIR/$SHC $FEAT_DIR/$SHC dvctrader 1 "" ind -1;
    fi; 
done;

