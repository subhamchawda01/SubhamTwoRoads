#!/bin/bash

USAGE="$0 SHORTCODE [LOOKBACK] [DATE] [RECOMPUTE=0]";

if [ $# -lt 1 ] ; then echo $USAGE; exit; fi
SHC=$1; 

LOOKBACK=5;
if [ $# -gt 1 ] ; then LOOKBACK=$2 ; fi 

DATE="TODAY";
if [ $# -gt 2 ] ; then DATE=$3; fi

RECOMPUTE=0;
if [ $# -gt 3 ] ; then RECOMPUTE=$4; fi

if [ "$DATE" == "YESTERDAY" ] ; then
  DATE=$(~/basetrade_install/bin/calc_prev_week_day $(date  +%Y%m%d));
fi

if [ "$DATE" == "TODAY" ] ; then
  DATE=$(date  +%Y%m%d);
fi

LOCKFILE="/mnt/sdf/locks/collect_feature_samples_"$SHC"_"$LOOKBACK".lock";
if [[ -e $LOCKFILE ]]; then echo "Lockfile already present: $LOCKFILE"; exit 0; fi;
touch $LOCKFILE;

trap "rm $LOCKFILE; exit" SIGHUP SIGINT SIGTERM

echo $HOME/basetrade_install/bin/collect_features_samples  $HOME/modelling/samples_features_configs/"$SHC"_config.txt $DATE $LOOKBACK $RECOMPUTE 0; 
$HOME/basetrade_install/bin/collect_features_samples  $HOME/modelling/samples_features_configs/"$SHC"_config.txt $DATE $LOOKBACK $RECOMPUTE 0;

echo $HOME/basetrade_install/scripts/get_periodic_l1_events_per_sec_past_ndays.sh $SHC $DATE $LOOKBACK $RECOMPUTE ;
$HOME/basetrade_install/scripts/get_periodic_l1_events_per_sec_past_ndays.sh $SHC $DATE $LOOKBACK $RECOMPUTE ;


if [[ -e $LOCKFILE ]]; then rm $LOCKFILE ; fi;

#chown -R dvctrader:infra /home/dvctrader/SampleData/"$SHC" ; 
#rsync -ravz /home/dvctrader/SampleData/"$SHC" dvcinfra@10.23.74.41:/apps/SampleData >/dev/null 2>&1 # SYNCS SAMPLEDATA  -> NAS1
