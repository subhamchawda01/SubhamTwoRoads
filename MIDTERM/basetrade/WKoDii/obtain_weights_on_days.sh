#!/bin/bash

if [ $# -lt 2 ]; then echo "USAGE: "$0" <current_date> <feature_file> [USE_LAST_DAY/ARIMA] [<distance_metric>]"; exit 1; fi;

curr_date=$1;
feature_file=$2;
model_type="ARIMA";
if [ $# -gt 2 ]; then model_type=$3; fi;
distance_metric="Mahalanobis";
if [ $# -gt 3 ]; then distance_metric=$4; fi;

#echo $HOME/basetrade/WKoDii/obtain_weights_on_days.py $curr_date -1 1 $feature_file $arima_paramfile $distance_metric ;
$HOME/basetrade/WKoDii/obtain_weights_on_days.py $curr_date -1 1 $feature_file $model_type $distance_metric ;

