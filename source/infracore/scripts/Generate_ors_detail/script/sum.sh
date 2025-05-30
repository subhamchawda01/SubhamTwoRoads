#!/bin/bash
readpath="/home/dvcinfra/important/Generate_ors_detail/Product_Details/"
declare -A month_product_cash
declare -A month_exec_cash
>/tmp/outfiledataJan
yyyymm=`date  +'%Y%m'`
for file in $readpath*
do
  filename=$(basename $file);
  [[ $filename == "$yyyymm"*"_cash" ]] || continue;
  day=`echo $filename| cut -d'_' -f1`
  cashFile=$file
  echo $day
  cat $cashFile| grep BATAINDIA >>/tmp/outfiledataJan
done


