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

EXEC=$HOME/basetrade_install/bin/get_avg_l1sz_on_day
DEST_DIR=/spare/local/tradeinfo/L1SizeInfo/
if [ ! -d $DEST_DIR ];
then
    mkdir -p $DEST_DIR 
fi
id_=`date +%s%n`;

ref_file=/spare/local/tradeinfo/dep_ses_tp.txt

for symbol in `cat $ref_file | grep -v "#" | awk '{ print $1 }'`;
do
    echo $symbol;
    ss=`grep $symbol $ref_file | awk '{ print $2 }'`;
    st=`grep $symbol $ref_file | awk '{ print $3 }'`;
    et=`grep $symbol $ref_file | awk '{ print $4 }'`;

    symbol_file_=$DEST_DIR"/"$symbol"_"$ss;

    echo $symbol" "$ss" "$st" "$et" "$symbol_file_;

    if [ -e $symbol_file_ ]; then
      grep $YYYYMMDD $symbol_file_ >/dev/null 2>/dev/null;
      if [ $? -eq 1 ]; then
      echo $symbol" "$ss;
        echo "Adding avg l1 size for $symbol for $ss for $YYYYMMDD";
        $EXEC $symbol $YYYYMMDD $st $et | awk -v d="$YYYYMMDD" '{ if($3 != 0) print $1" "d" "$3;}' |  cat - $symbol_file_ | sort -nk2 -r | awk 'BEGIN{nextdate = 20991212}{if(nextdate > $2){print $1" "$2" "$3}; nextdate = $2}' > $DEST_DIR"/temp_$symbol$ss$id_";
        mv $DEST_DIR"/temp_$symbol$ss$id_" $symbol_file_;
      fi;
    else
      echo "Adding avg l1 size for $symbol for $ss for $YYYYMMDD";
      $EXEC $symbol $YYYYMMDD $st $et | awk -v d="$YYYYMMDD" '{ if($3 != 0) print $1" "d" "$3;}'| sort -nk2 -r | awk 'BEGIN{nextdate = 20991212}{if(nextdate > $2){print $1" "$2" "$3}; nextdate = $2}' >> $symbol_file_;
   fi; 
done

$HOME/basetrade/scripts/sync_dir_to_all_machines.pl $DEST_DIR;
