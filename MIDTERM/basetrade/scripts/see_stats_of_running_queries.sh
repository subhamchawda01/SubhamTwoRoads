#!/bin/bash

if [ $# -lt 1 ] ; then
    echo "need HOST";
    exit;
fi
HOST=$1; shift;

if [ $# -lt 1 ] ; then
    echo "need SHC";
    exit;
fi
SHC=$1; shift;

NUM_PAST_DAYS=60;
if [ $# -ge 1 ] ; then
    NUM_PAST_DAYS=$1; shift;
fi

TIMEPERIOD="US_MORN_DAY";
if [ $# -ge 1 ] ; then
    TIMEPERIOD=$1; shift;
fi

if [ -e $HOME/crontabs/crontab.$HOST ] ; then
    for name in `grep start $HOME/crontabs/crontab.$HOST | awk '{print $9}' | rev | cut -d _ -s --complement -f1 | cut -d / -s -f1 | rev `; 
    do 
	$HOME/basetrade/scripts/ss_noc.sh $SHC $TIMEPERIOD $NUM_PAST_DAYS | grep $name ; 
    done
fi
