#!/bin/bash

USAGE="$0 SHORTCODE TIMEOFDAY TRADING_END_YYYYMMDD NUM_PAST_DAYS [VOL/PNL/TTC/PPT/MAXDD] [0|1(TIMEOFDAY should be treated as foldername)]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHORTCODE=$1; shift;
TIMEPERIOD=$1; shift;

TRADING_END_YYYYMMDD="TODAY-1";
if [ $# -ge 1 ] ; 
then 
    TRADING_END_YYYYMMDD=$1; shift;
fi

NUM_PAST_DAYS=5;
if [ $# -ge 1 ] ; 
then 
    NUM_PAST_DAYS=$1; shift;
fi

COL=2;

if [ $# -ge 1 ] ;
then
    col=$1; shift;
    case $col in
	VOL|vol)
	    COL=2;
	    ;;
	PNL|pnl)
	    COL=1;
	    ;;
	TTC|ttc)
	    COL=3;
	    ;;
	PPT|ppt)
	    COL=4;
	    ;;
	MAXDD|maxdd)
	    COL=5;
	    ;;
	*)
	    ;;
    esac
fi

#time-period is actually folder name - otherwise creates problem while pruning
folder_name=0;
if [ $# -ge 1 ] ;
then
    folder_name=$1; shift;
fi


~/basetrade/ModelScripts/summarize_strats_for_day_no_outsample_check.pl $SHORTCODE $TIMEPERIOD $TRADING_END_YYYYMMDD $NUM_PAST_DAYS INVALIDFILE $folder_name| awk '{ if ( $2 == "_fn_" ) printf "%5s %7s %4s %5s %5s %s\n" , $3 , $5 , $9 , $12 , $17 , $2 ; else if ( NF > 15 ) { printf "%5d %7d %4d %+4.2f %5d %s\n" , $3 , $5 , $9 , $12 , $17 , $2 } }' | sort -k $COL -g
