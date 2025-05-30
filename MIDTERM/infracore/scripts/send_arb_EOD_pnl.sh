#!/bin/bash

#export NEW_GCC_LIB=/usr/local/lib
#export NEW_GCC_LIB64=/usr/local/lib64
#export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

FILE="$HOME/EODPnl/ALL_ARB_TRADES";
PNL_SCRIPT="$HOME/infracore/scripts/see_ors_pnl.pl";

if [ ! -d $HOME/EODPnl ] ;
then
    mkdir $HOME/EODPnl;
fi

> $FILE;

YYYYMMDD=$(date "+%Y%m%d");

if [ $# -ge 1 ] ;
then
    YYYYMMDD=$1;
fi

PREV_DATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;

rsync -avz  --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/MS005/trades.$YYYYMMDD   $HOME/trades/bmf11_arb.trades.$YYYYMMDD
cat $HOME/trades/bmf11_arb.trades.$YYYYMMDD >> $FILE;

perl -w $PNL_SCRIPT 'R' $FILE $YYYYMMDD 0 > /apps/data/MFGlobalTrades/EODPnl/ors_arb_pnls_$YYYYMMDD".txt"

cat /apps/data/MFGlobalTrades/EODPnl/ors_arb_pnls_$YYYYMMDD".txt" 

scp /apps/data/MFGlobalTrades/EODPnl/ors_arb_pnls_$YYYYMMDD".txt" dvcinfra@10.23.74.40:/apps/data/MFGlobalTrades/EODPnl/ >/dev/null 2>/dev/null
