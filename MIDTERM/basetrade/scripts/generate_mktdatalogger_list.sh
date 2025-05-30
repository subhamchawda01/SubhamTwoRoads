#!/bin/bash

if [ $# -lt 4 ]; then
  echo "USAGE: $0 <product> <start date> <end date> <directory>";
  exit 0;
fi;

product=$1;
start_date=$2;
end_date=$3;
dt=$end_date;
directory=$4;


if [ ! -d "$directory" ]; then
  mkdir -p $directory;
else
  echo "Directory already exists";
  exit 0;     #should we exit?
fi;

while [ $dt -ge $start_date ];do 
  ~/basetrade_install/bin/mkt_trade_logger SIM $product $dt 100 NO UTC_0001 UTC_2359 EVT_10 2>/dev/null | awk '{ if ( $2 == "OnMarketUpdate" ) { print $1%86400*1000,"UPDATE",$12,$13,$14,$15,$16,$17,$18, $19} else { print $1%86400*1000,"TRADE",$5,$4,$6,$7,$18,$19,$20,$21,$22,$23,$24,$25} } '>$directory/datalog_"$dt";
  echo $dt "$directory/datalog_$dt" >> "$directory/all_datalog";
  dt=`~/basetrade_install/bin/calc_prev_week_day $dt`;
done
