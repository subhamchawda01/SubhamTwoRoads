#!/bin/bash

if [ $# -ne 1 ];
then 
    echo "$0 DATE";
    exit;
fi
YYYYMMDD=$1;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

EXEC=$HOME/basetrade_install/bin/get_periodic_l1events_on_day
DEST_DIR=/spare/local/tradeinfo/L1EventsInfo/
if [ ! -d $DEST_DIR ];
then
    mkdir -p $DEST_DIR 
fi
id_=`date +%s%n`;

ref_file=/spare/local/tradeinfo/sources.txt

for symbol in `cat $ref_file | grep -v "#"`;
do
    echo $symbol;
    symbol_file_=$DEST_DIR"/"$symbol;

    if [ -e $symbol_file_ ]; then
      if ! grep -lq $YYYYMMDD $symbol_file_ ; then
        echo "Adding avg l1 events for $symbol for $YYYYMMDD";
        l1ev_persec=`$EXEC $symbol $YYYYMMDD 2>/dev/null | awk '{SUM += $2;} END{if(NF>0) { print SUM/(NF*900); } }'`;
        if [ -n $l1ev_persec ]; then 
          echo $YYYYMMDD $l1ev_persec |  cat - $symbol_file_ | sort -nk1 -r | awk 'BEGIN{nextdate = 20991212}{if(nextdate > $2){print $1" "$2}; nextdate = $1}' > $DEST_DIR"/temp_$symbol$id_";
          mv $DEST_DIR"/temp_$symbol$id_" $symbol_file_;
        fi;
      fi;
    else
      echo "Adding avg l1 events for $symbol for $YYYYMMDD";
      l1ev_persec=`$EXEC $symbol $YYYYMMDD 2>/dev/null  | awk '{SUM += $2;} END{if(NF>0) { print SUM/(NF*900); } }'`;
      if [ -n $l1ev_persec ]; then
        echo $YYYYMMDD $l1ev_persec > $symbol_file_;
      fi;
   fi; 
done

$HOME/basetrade/scripts/sync_dir_to_all_machines.pl $DEST_DIR;
