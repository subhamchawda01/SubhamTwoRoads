#!/bin/bash

USAGE="$0 SHORTCODE TIMEOFDAY [NUM_PAST_DAYS=5] [TILL_DATE=TODAY-1] [EXTENDED=0]";
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

if [ $EXTENDED -gt 0 ] ;
then
    ~/basetrade/ModelScripts/summarize_strats_for_day.pl $SHORTCODE $TIMEPERIOD $TILL_DATE $NUM_PAST_DAYS | grep -v MAXDD | awk '{ if ( NF > 15 ) { printf "%d %d %.4f %s %.4f %.2f %d\n", $3, $5, ($3 / $17), $2 , $4 , $6 , $10} }' # TTC changed from $9 to $10 ( norm-tcc )
else
    ~/basetrade/ModelScripts/summarize_strats_for_day.pl $SHORTCODE $TIMEPERIOD $TILL_DATE $NUM_PAST_DAYS | grep -v MAXDD | awk '{ if ( NF > 15 ) { printf "%d %d %.4f %s\n", $3, $5, ($3 / $17), $2 } }' #| sort -rg -k3
fi
