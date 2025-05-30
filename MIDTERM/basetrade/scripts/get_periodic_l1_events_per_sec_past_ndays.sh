#!/bin/bash

if [ $# -lt 2 ];
then 
    echo "$0 SHC END-DATE [NUMDAYS] [REPLACE=0]";
    exit;
fi

symbol=$1;

YYYYMMDD=$2;
if [ $YYYYMMDD = "TODAY" ] ; then YYYYMMDD=$(date "+%Y%m%d"); fi

sfreq=1;
if [ $# -gt 2 ]; then sfreq=$3; fi

replace=0;
if [ $# -gt 3 ]; then replace=$4; fi
  

EXEC=$HOME/basetrade_install/bin/get_periodic_l1events_on_day
DEST_DIR=$HOME/SampleData/
if [ ! -d $DEST_DIR ]; then mkdir -p $DEST_DIR ; fi

ed=$YYYYMMDD;
sd=`~/basetrade_install/bin/calc_prev_week_day $ed $sfreq`;
while [ "$ed" -gt "$sd" ] ; do 
    echo $ed;
    symbol_file_=$DEST_DIR/$symbol/$ed/L1EventsPerSecond.txt;
    symbol_dir_=`dirname $symbol_file_`;
    if [ ! -d $symbol_dir_ ]; then mkdir -p $symbol_dir_ ; fi
    
    if [ ! -e $symbol_file_ ] || [ $replace -eq 1 ]; then
      $EXEC $symbol $ed 1 2>/dev/null > $symbol_file_;
      if [ ! -s $symbol_file_ ]; then rm $symbol_file_ ; fi
    fi;
    ed=`~/basetrade_install/bin/calc_prev_week_day $ed 1`;
done
