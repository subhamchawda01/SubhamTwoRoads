#!/bin/bash

if [ $# -ne 3 ] && [ $# -ne 4 ]; then
  echo "USAGE: <SCRIPT> <TRADE_FILE> <YYYYMMDD> <TIME> <NSE_SBIN(OPTIONAL)>"
  exit
fi

get_pnl_product() {
    product_=$1
    trade_file_pnl="/tmp/trade_file_pnl_${product_}"
    mkt_trade_output="/tmp/mkt_trade_output_${product_}"
    key_word=${product_}".";
    echo "$key_word $time_ $trade_file"
    grep -a "$key_word" $trade_file | awk -v shortcode=${product_} -v time_stamp=${time_} '{if ($1 <= time_stamp) { total_price=$5*$6; if ($4 == "B") {pnl+=total_price; total_qty+=$5;} else {pnl-=total_price; total_qty-=$5;}} next;} END {printf "%s %f %d %f\n",shortcode,time_stamp,total_qty,pnl}' > $trade_file_pnl
    echo "$product_ $trade_file_pnl"

    echo "$mkt_trade_logger SIM $product_ $date_ > $mkt_trade_output"
    ssh dvctrader@52.90.0.239 "$mkt_trade_logger SIM $product_ $date_ > $mkt_trade_output"
    echo "scp dvctrader@52.90.0.239:$mkt_trade_output $mkt_trade_output"
    scp dvctrader@52.90.0.239:$mkt_trade_output $mkt_trade_output
    awk 'NR == FNR {shortcode_=$1; time_=$2; qty_=$3; price_=$4; next;} { if ($1 <= time_) {mean_price=(($(NF-3)+$(NF-2)) / 2 * qty_ * -1) + price_; printf"%s %f %f\n",shortcode_,time_,mean_price} }' $trade_file_pnl $mkt_trade_output | tail -1 >> $pnl_output
    ssh dvctrader@52.90.0.239 "rm $mkt_trade_output"
    rm $mkt_trade_output $trade_file_pnl 
}

trade_file=$1;
date_=$2;
time_=$3;
mkt_trade_logger="/home/dvctrader/stable_exec/mkt_trade_logger_20220202_finnifty"
pnl_output="/tmp/pnl_output_$date_"
>$pnl_output

if [ $# -eq 3 ]; then
  for product in `awk -F'[. ]' '{print $4}' $trade_file | sort | uniq`; do
    get_pnl_product $product &
    sleep 5;
    total_proc=`ssh dvctrader@52.90.0.239 "ps aux | grep mkt_trade_logger_20220202_finnifty | grep -v grep | wc -l"`
    while [ $total_proc -gt 7 ]
    do
      echo "sleep 1m"
      sleep 1m;
      total_proc=`ssh dvctrader@52.90.0.239 "ps aux | grep mkt_trade_logger_20220202_finnifty | grep -v grep | wc -l"`
      echo "total process $total_proc"
    done 
  done
  echo "OUTPUT: $pnl_output"
  
elif [ $# -eq 4 ]; then
  product=${4};
  get_pnl_product $product
  echo "OUTPUT: $pnl_output"
fi

