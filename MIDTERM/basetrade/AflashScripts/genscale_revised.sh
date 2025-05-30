#!/bin/bash

if [ $# -lt 5 ]; then echo "$0 <shortcode> <event_datfile> <event_id> <beta2/beta5/beta10> <datum1_id> <datum2_id> .."; exit; fi

shc=$1;
datfile=$2;
event_id=$3;
betamins=$4;
ev_ids=( "${@:5}" );
ev_no=${#ev_ids[@]};

betaout=`Rscript ~/basetrade/AflashScripts/findScale_revised.R $datfile $shc $shc/pxchange.dat | grep $betamins`;
betawords=( $betaout );

outline="$shc $event_id";
for id in `seq 1 $ev_no` ; do outline=$outline" "${ev_ids[$((id-1))]}":"${betawords[$id]} ; done;

echo $outline;
