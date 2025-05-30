#!/bin/bash

if [ $# -lt 1 ] ; then
echo "need SHC";
fi
SHC=$1; shift;

FNAME=/spare/local/tradeinfo/datageninfo/high_volume_days.$SHC;
sort -g -k1 $FNAME > $FNAME"_old";
$HOME/basetrade/scripts/get_high_volume_days.pl $SHC /spare/local/tradeinfo/datageninfo 60 20 ;
sort -g -k1 $FNAME > $FNAME"_new";
cat $FNAME"_old" $FNAME"_new" | sort -g | uniq > $FNAME
rm -f $FNAME"_old" $FNAME"_new";
