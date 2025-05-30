#!/bin/bash

USAGE="$0 SHORTCODE STRATLISTFILE DATES_FILE [EXTENDED=0] [SKIP_DATES_FILE=INVALIDFILE] [USE_MAX_LOSS=0]";
if [ $# -lt 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHORTCODE=$1; shift;
STRATLISTFILE=$1; shift;
DATES_FILE=$1; shift;

EXTENDED=0;
if [ $# -gt 0 ] ;
then
    EXTENDED=$1; shift;
fi

SKIP_DATES_FILE="INVALIDFILE";
if [ $# -gt 0 ] ;
then
    SKIP_DATES_FILE=$1; shift;
fi

USE_MAX_LOSS=0;
if [ $# -gt 0 ] ;
then
    USE_MAX_LOSS=$1; shift;
fi

if [ $EXTENDED -gt 0 ] ;
then
    ~/basetrade/ModelScripts/summarize_strats_for_day_pickstrats.pl $SHORTCODE $STRATLISTFILE $DATES_FILE $SKIP_DATES_FILE 0 $USE_MAX_LOSS | grep -v MAXDD | awk '{ if ( NF > 18 ) { printf "%d %d %.4f %s %.4f %.2f %d %.2f %d %d\n", $3, $5, ($3 / $17), $2 , $4 , $6 , $10 , $19 , $20, $23 } }' # TTC changed from $9 to $10 ( norm
else
    echo "hii\n";
    ~/basetrade/ModelScripts/summarize_strats_for_day_pickstrats.pl $SHORTCODE $STRATLISTFILE $DATES_FILE $SKIP_DATES_FILE 0 $USE_MAX_LOSS | grep -v MAXDD | awk '{ if ( NF > 18 ) { printf "%d %d %.4f %s\n", $3, $5, ($3 / $17), $2 } }' #| sort -rg -k3
fi
