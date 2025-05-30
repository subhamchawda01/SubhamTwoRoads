#!/bin/bash

USAGE="$0 SHORTCODE TIMEOFDAY [NUM_PAST_DAYS=5] [TILL_DATE=TODAY-1] [EXTENDED=0] [SKIP_DATE_FILE=INVALIDFILE] [USE_MAX_LOSS=0]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHORTCODE=$1; shift;
TIMEPERIOD=$1; shift;

NUM_PAST_DAYS=5;
if [ $# -gt 0 ] ; 
then 
    NUM_PAST_DAYS=$1; shift;
fi

TILL_DATE="TODAY-1";
if [ $# -gt 0 ] ;
then
    TILL_DATE=$1; shift;
fi

EXTENDED=0;
if [ $# -gt 0 ] ;
then
    EXTENDED=$1; shift;
fi

SKIP_DATE_FILE="INVALIDFILE";
if [ $# -gt 0 ] ;
then
    SKIP_DATE_FILE=$1; shift;
fi

USE_MAX_LOSS=0;
if [ $# -gt 0 ] ;
then
    USE_MAX_LOSS=$1; shift;
fi


if [ $EXTENDED -gt 0 ] ;
then
    ~/basetrade/ModelScripts/summarize_strats_for_day_no_outsample_check.pl $SHORTCODE $TIMEPERIOD $TILL_DATE $NUM_PAST_DAYS $SKIP_DATE_FILE 0 $USE_MAX_LOSS | grep -v MAXDD | awk '{ if ( NF > 18 ) { printf "%d %d %.4f %s %.4f %.2f %d %.2f %d %d\n", $3, $5, ($3 / $17), $2 , $4 , $6 , $10 , $19 , $20, $23 } }' # TTC changed from $9 to $10 ( norm
else
    ~/basetrade/ModelScripts/summarize_strats_for_day_no_outsample_check.pl $SHORTCODE $TIMEPERIOD $TILL_DATE $NUM_PAST_DAYS $SKIP_DATE_FILE 0 $USE_MAX_LOSS | grep -v MAXDD | awk '{ if ( NF > 18 ) { printf "%d %d %.4f %s\n", $3, $5, ($3 / $17), $2 } }' #| sort -rg -k3
fi
