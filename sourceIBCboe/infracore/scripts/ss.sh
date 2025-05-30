#!/bin/bash

USAGE="$0 SHORTCODE TIMEOFDAY";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHORTCODE=$1; shift;
TIMEPERIOD=$1; shift;

NUM_PAST_DAYS=3;
if [ $# -gt 0 ] ; 
then 
    NUM_PAST_DAYS=$1; shift;
fi

~/infracore/ModelScripts/summarize_strats_for_day.pl $SHORTCODE $TIMEPERIOD TODAY-1 $NUM_PAST_DAYS | grep -v MAXDD | awk '{ if ( NF > 15 ) { printf "%d %d %.2f %s\n", $3, $5, ($3 / $16), $2 } }' #| sort -rg -k3
