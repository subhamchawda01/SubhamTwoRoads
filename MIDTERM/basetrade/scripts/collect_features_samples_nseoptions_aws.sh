#!/bin/bash

USAGE="$0 NSE_OPTION_PREFIX [LOOKBACK] [DATE]";

if [ $# -lt 1 ] ; then echo $USAGE; exit; fi
SHC_PREF=$1; 

LOOKBACK=5;
if [ $# -gt 1 ] ; then LOOKBACK=$2 ; fi 

DATE="YESTERDAY";
if [ $# -gt 2 ] ; then DATE=$3; fi

if [ "$DATE" == "YESTERDAY" ] ; then
  DATE=$(~/basetrade_install/bin/calc_prev_week_day $(date  +%Y%m%d));
fi

if [ "$DATE" == "TODAY" ] ; then
  DATE=$(date  +%Y%m%d);
fi

TEMPDIR="/spare/local/temp";

SRC_ILIST=$HOME/modelling/samples_features_configs/option_config.txt;

for putcall in P C; do
  for optiontype in A O1 O2 O3 I1 I2 I3; do
    SHC=$SHC_PREF"_"$putcall"0_"$optiontype;

    TGT_ILIST=$TEMPDIR/$SHC"_config.txt";

    LOCKFILE="/mnt/sdf/locks/collect_feature_samples_"$SHC"_"$LOOKBACK".lock";
    
    if [[ -e $LOCKFILE ]]; then exit 0; fi;
    touch $LOCKFILE;
    
    awk -vshc=$SHC '{if ($1=="SHC") { $2=shc; } print $_; }' $SRC_ILIST > $TGT_ILIST;

    echo $HOME/basetrade_install/bin/collect_features_samples  $TGT_ILIST $DATE $LOOKBACK ;
    $HOME/basetrade_install/bin/collect_features_samples  $TGT_ILIST $DATE $LOOKBACK ;

    #chown -R dvctrader:infra /home/dvctrader/SampleData/"$SHC" ; 
    if [[ -e $TGT_ILIST ]]; then rm $TGT_ILIST ; fi;
    if [[ -e $LOCKFILE ]]; then rm $LOCKFILE ; fi;
  done
done
