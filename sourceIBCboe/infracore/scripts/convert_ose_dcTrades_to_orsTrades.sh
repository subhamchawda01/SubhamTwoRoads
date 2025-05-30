#! /bin/bash

usage="$0 ose_dc_trade_file YYYYMMDD"
if [ $# -ne 2 ] ; 
then 
    echo $usage;
    exit;
fi

dc_file=$1
date=$2

declare -A dc_symbol_to_ors_symbol_map ;
dc_symbol_to_ors_symbol_map["JGBL"]="JGBL";
dc_symbol_to_ors_symbol_map["NK225"]="NK";
dc_symbol_to_ors_symbol_map["NK225M"]="NKM";
dc_symbol_to_ors_symbol_map["TOPIX"]="TOPIX";
awk -F"," '{print $12" "$19" "$29" "$28" "0}' $dc_file | grep -v "SeriesID"> ose_trades.$date
sed -i s/FUT_//g ose_trades.$date

for sym in `awk '{print $1}' ose_trades.$date | sort | uniq`
do
  dc_symbol=`echo $sym | awk -F"_" '{print $1}'`
  exp=`echo  $sym |awk -F"_" '{print $2}'`
  ors_symbol=${dc_symbol_to_ors_symbol_map[$dc_symbol]}"$exp"
  echo $dc_symbol $ors_symbol
  sed -i s/"$sym"/"$ors_symbol"/g ose_trades.$date
done

sed -i s/Ask/1/g ose_trades.$date
sed -i s/Bid/0/g ose_trades.$date

#for topix and jgbl, px needs to be *100
awk '{if(substr($1,1,5) == "TOPIX" || substr($1,1,4) == "JGBL") print $1" "$2" "$3" "$4*100" "$5; else print $0}' ose_trades.$date > ose_trades.$date_tmp
mv ose_trades.$date_tmp ose_trades.$date
tr ' ' '\001' < ose_trades.$date > ose_trades.$date_tmp
mv ose_trades.$date_tmp ose_trades.$date

