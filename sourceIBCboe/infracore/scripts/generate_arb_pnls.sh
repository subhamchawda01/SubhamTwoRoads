#!/bin/bash

query_list_file=/spare/local/tradeinfo/arb_queries
queries=`cat $query_list_file | awk '{print $1}'`

tradingdate=`date +%Y%m%d`
if [ $# -ge 1 ] ;
then
    tradingdate=$1;
fi

yyyy=${tradingdate:0:4}
mm=${tradingdate:4:2}
dd=${tradingdate:6:2}

total_volume=0
total_pnl=0

echo -e "Pnl\tVolume\tQueryId\tProducts\tDescription"
for id in $queries
do
    tradesfile="/NAS1/logs/QueryTrades/$yyyy/$mm/$dd/trades.$tradingdate"."$id"
    if [ -f $tradesfile ]
    then
        #tail -1 $tradesfile
        volume=`awk '{total += $5} END{print total}' $tradesfile`
        pnl=`tail -1 $tradesfile | awk '{print $18}'`
        total_volume=$(($total_volume+$volume))
        total_pnl=$((total_pnl+$pnl))
        echo -e "$pnl\t$volume\t`grep $id $query_list_file`"
    fi
done

echo -e "\n\n"
echo -e "Total\t$total_pnl\t$total_volume"
