#!/bin/bash

USAGE="$0 SHORTCODE DATE START_TIME END_TIME UNIQ_ID"
if [ $# -lt 4 ] ;
then
  echo $USAGE;
  exit;
fi

SHC=$1
DATE=$2
START_TIME=$3
END_TIME=$4
UNIQID=$5

DGEN_EXEC=/home/pengine/prod/live_execs/datagen_online
FEATURE_FILE=/spare/local/files/"$SHC"_sample_feature_config.txt

$DGEN_EXEC $FEATURE_FILE $DATE $START_TIME $END_TIME $UNIQID STATS_SAMPLES 1000 0 0 0 &
