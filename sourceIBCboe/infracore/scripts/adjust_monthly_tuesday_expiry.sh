#!/bin/bash


if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 YYYYMMDD(MONDAY)" ;
  exit
fi

date_=$1


trade_fu0_fin=`/home/dvctrader/stable_exec/mkt_trade_logger_20220202_finnifty SIM NSE_FINNIFTY_FUT0 $date_ | grep -i trade  | tail -1 | awk '{print $7}'`
trade_fut1_fin=`/home/dvctrader/stable_exec/mkt_trade_logger_20220202_finnifty SIM NSE_FINNIFTY_FUT1 $date_ | grep -i trade  | tail -1 | awk '{print $7}'`

ratio_fin=$(echo "$trade_fut1_fin/$trade_fu0_fin" | bc -l)


echo "Ratio FIN $ratio_fin"



trade_fu0_mid=`/home/dvctrader/stable_exec/mkt_trade_logger_20220202_finnifty SIM NSE_MIDCPNIFTY_FUT0 $date_ | grep -i trade  | tail -1 | awk '{print $7}'`
trade_fut1_mid=`/home/dvctrader/stable_exec/mkt_trade_logger_20220202_finnifty SIM NSE_MIDCPNIFTY_FUT1 $date_ | grep -i trade  | tail -1 | awk '{print $7}'`

ratio_mid=$(echo "$trade_fut1_mid/$trade_fu0_mid" | bc -l)

echo "Ratio MID $ratio_mid"

cp /spare/local/BarData/FINNIFTY /spare/local/BarData/FINNIFTY_bkp_${date_}
cp /spare/local/BarData/FINNIFTY /spare/local/BarData/FINNIFTY_bkp_${date_}

echo "Updating FINNIFTY"
/home/dvctrader/EOD_SCRIPTS/corp_adj_new.sh FINNIFTY $ratio_fin
echo "Updating MIDCAP"
/home/dvctrader/EOD_SCRIPTS/corp_adj_new.sh MIDCPNIFTY $ratio_mid
