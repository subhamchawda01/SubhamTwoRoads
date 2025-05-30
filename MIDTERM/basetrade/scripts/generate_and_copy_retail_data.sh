#!/bin/bash

if [ $# -lt 1 ]; then echo "USAGE: $0 <date>"; exit 0; fi;

dt=$1;
if [ $dt == "TODAY" ]; then dt=`date +%Y%m%d`; fi;
yyyy=${dt:0:4}; mm=${dt:4:2}; dd=${dt:6:2}; 

logfile="/NAS1/logs/QueryLogs/$yyyy/$mm/$dd/log.$dt.30892.gz";
uid=`date +%N`;
tmp_dir="/tmp/RETAILLoggedData/BRZ";
tmp_file="/tmp/retail_trades_$dt"
`rm -rf $tmp_file`;
`rm -rf $tmp_dir`;
if [ -s $logfile ]; then 
  zgrep "ProcessORSReply  Struct" $logfile  | awk '{bs="B"; if($21==1){bs="S"}; print $9 ,$5, $27, bs , $7;}' | column -t > $tmp_file; 
  if [ -s $tmp_file ]; then 
    $HOME/basetrade_install/bin/retail_logger $dt $tmp_file $tmp_dir;
    gzip $tmp_dir/*/*/*/*;
    chmod 755 -R $tmp_dir;
    chmod 755 $tmp_file;
    scp $tmp_file dvcinfra@10.23.74.40:/apps/data/Retail_Trades/ ;
    scp -r $tmp_dir dvcinfra@10.23.74.40:/apps/data/RETAILLoggedData/ ;
  fi;     
fi; 
`rm -rf $tmp_file`;
`rm -rf $tmp_dir`;

