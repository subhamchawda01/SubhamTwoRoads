#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

pscript=/home/pengine/prod/live_scripts/

$pscript/fetch_abnamro_files.sh $YYYYMMDD
$pscript/fetch_edf_files.sh $YYYYMMDD

#for newedge, get current date's broker file as well(which is incomplete but will be downloaded again tomorrow)
$pscript/fetch_newedge_files.sh $(date "+%Y%m%d")
$pscript/fetch_newedge_files.sh $YYYYMMDD

$pscript/fetch_plural_files.sh $YYYYMMDD
$pscript/fetch_rencap_files.sh $YYYYMMDD
