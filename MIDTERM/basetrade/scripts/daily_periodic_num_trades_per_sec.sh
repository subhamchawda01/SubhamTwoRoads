#!/bin/bash

if [ $# -lt 1 ];
then 
    echo "$0 DATE [REPLACE=0]";
    exit;
fi

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ; then YYYYMMDD=$(date "+%Y%m%d"); fi

replace=0;
if [ $# -gt 1 ]; then replace=$2; fi
  

EXEC=$HOME/basetrade_install/bin/get_periodic_numtrades_on_day
DEST_DIR=$HOME/SampleData/
if [ ! -d $DEST_DIR ]; then mkdir -p $DEST_DIR ; fi

ref_file=/spare/local/tradeinfo/sources.txt

for symbol in `cat $ref_file | grep -v "#"`;
do
    echo $symbol;
    symbol_file_=$DEST_DIR/$symbol/$YYYYMMDD/NumTradesPerSecond.txt;
    symbol_dir_=`dirname $symbol_file_`;
    if [ ! -d $symbol_dir_ ]; then mkdir -p $symbol_dir_ ; fi
    
    if [ ! -e $symbol_file_ ] || [ $replace -eq 1 ]; then
      $EXEC $symbol $YYYYMMDD 1 2>/dev/null > $symbol_file_;
      if [ ! -s $symbol_file_ ]; then rm $symbol_file_ ; fi
    fi;
done
