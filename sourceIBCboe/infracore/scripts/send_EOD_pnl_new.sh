#!/bin/bash

PNL_SCRIPT="$HOME/infracore/scripts/get_EOD_pnl.sh";

YYYYMMDD=$(date "+%Y%m%d");

if [ $# -ge 1 ] ;
then
    YYYYMMDD=$1;
fi

$PNL_SCRIPT $YYYYMMDD 1 > /apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD".txt" 2>/dev/null

cat /apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD".txt"

#sync to file-server 
scp /apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD".txt" dvcinfra@10.23.74.41:/reports/apps/MFGlobalTrades/EODPnl/ >/dev/null 2>/dev/null
