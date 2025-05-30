#!/bin/bash

date=`date +%Y%m%d`
count_=`ssh dvctrader@44.202.186.243  "ps aux | grep log_crypto_mktdata_from_websocket_v3_multithread.py" |   grep -v grep | wc -l`
echo "Current Count $count_"
if [[ $count_ -lt 1 ]]; then
  echo "Sending Mail for not running"
  echo "" | mailx -s "CoinBase Crypto Logger Not running" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
  ssh dvctrader@44.202.186.243 "bash; taskset -c 35,36 /home/dvctrader/.pyenv/versions/3.9.13/bin/python3 /home/dvctrader/raghu/CryptoCode/CryptoCode/CoinBase/log_crypto_mktdata_from_websocket_v3_multithread.py --info True >/spare/local/logs/coinbase_loggin_data_rerun_${date} 2>&1" & 
fi
