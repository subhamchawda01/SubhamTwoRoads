#!/bin/bash

if [ "$#" -ne 1 ] ; then
  echo "USAGE: SCRIPT <next_working_date_YYYYMMDD>"
  exit
fi

dt=`date +"%Y%m%d"`
dd=${dt:6:2}
yyyy=${dt:0:4}
mstr=`date -d $dt +%b`
#dt="20201112"
gsm_file="gsm-latest.csv"
nifty_50_file="MW-NIFTY-50-$dd-$mstr-$yyyy.csv"
nifty_next_50_file="MW-NIFTY-NEXT-50-$dd-$mstr-$yyyy.csv"
next_working_date=$1;

echo "$gsm_file"
#awk 'NR>1' "$gsm_file" | awk -F "|" '{print $1}' > "productlist$dt.txt"
awk 'NR>6' "$gsm_file" | awk -F "\"*,\"*" '{print $2}' > "productlist$dt.txt"

echo "$nifty_50_file"
#awk 'NR>1' "$nifty_50_file" | awk -F "\"*,\"*" '{ if(substr($1,2) != "NIFTY NEXT 50" && substr($1,2) != "NIFTY 50") print substr($1,2)}'  >> "productlist$dt.txt"
awk 'NR>14' "$nifty_50_file" | awk -F "\"*,\"*" '{ if(substr($1,2) != "NIFTY NEXT 50" && substr($1,2) != "NIFTY 50") print substr($1,2)}'  >> "productlist$dt.txt"

echo "$nifty_next_50_file"
awk 'NR>14' "$nifty_next_50_file" | awk -F "\"*,\"*" '{ if(substr($1,2) != "NIFTY NEXT 50" && substr($1,2) != "NIFTY 50") print substr($1,2)}' >> "productlist$dt.txt"

cat "productlist$dt.txt" | sort | uniq > "data_nifty_next.$dt"

scp "data_nifty_next.$dt" dvcinfra@10.23.5.26:/NAS1/data/MFGlobalTrades/ind_pnls/CM/trans_add_products
scp "data_nifty_next.$dt" dvcinfra@10.23.5.26:/NAS1/data/MFGlobalTrades/ind_pnls/CM/trans_add_products/data_nifty_next.$next_working_date

rm -f "productlist$dt.txt" $gsm_file $nifty_50_file $nifty_next_50_file 


