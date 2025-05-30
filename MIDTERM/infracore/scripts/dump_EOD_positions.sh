#!/bin/bash

DUMP_OVN_PNL_SCRIPT="/home/pengine/prod/live_scripts/see_ors_pnl_lopr.pl";
TMX_POS_COMPUTE="/home/pengine/prod/live_scripts/compute_tmx_positions.pl";

YYYYMMDD=$(date "+%Y%m%d");

if [ $# -ge 1 ] ;
then
    YYYYMMDD=$1;
fi

#Not using scp due to failures
scp -q 10.23.74.51:/tmp/YESTERDAY_DATE /tmp/YESTERDAY_DATE  ;
FILE="/spare/local/ORSlogs/TMXATR/BDMATR/atr_trades.$YYYYMMDD" ;

perl -w $DUMP_OVN_PNL_SCRIPT $FILE 1 > /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp.txt 2>/dev/null 
cp /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp.txt /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD".txt    
rm /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp.txt

