#!/bin/bash

USAGE="$0 SHORTCODE_FILE METRIC_FILE EZONE_STRING END_DATE LOOKBACK_DAYS EVENT_START_TIME EVENT_END_TIME RANDOM_START_TIME RANDOM_END_TIME <CORR INDEP SHORTCODE>";

if [ $# -lt 9 ];
then
    echo $USAGE;
    exit;
fi


shortcode_file=$1;
metric_file=$2;
ezone_string=$3;
end_date=$4;
look_back_days=$5;
start_time=$6;
end_time=$7;
random_start_time=$8;
random_end_time=$9;
HOME=`echo $HOME`;

if [ $# -eq 10 ];
then 
   indep_shortcode=$10;
fi

#event_date_file = "/media/shared/ephemeral16/dvctrader/temp_ebt_dates_"$((1 + RANDOM % 10000));

#`$HOME/basetrade/scripts/get_dates_for_traded_ezone.pl $ezone_string $start_date $end_date>$event_date_file`
for sh in `cat $shortcode_file` ;do echo $sh; for metric in `cat $metric_file`;do  event_data=`$HOME/basetrade/scripts/get_avg_samples.pl $sh  $end_date $look_back_days $start_time $end_time $ezone_string $metric`;random_data=`$HOME/basetrade/scripts/get_avg_samples.pl $sh  $end_date $look_back_days $random_start_time $random_end_time $ezone_string $metric`;printf "$metric event_data:$event_data \t random_data:$random_data\n";done;done
    
