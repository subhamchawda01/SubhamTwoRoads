#!/bin/bash

declare -A dict
declare -A symbol_settle

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 YYYYMMDD[Day before expiry] " ;
  exit ;
fi
date=$1

next_working_day=`/home/pengine/prod/live_execs/update_date $date N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]]
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done

ksOut=/spare/local/MidtermStrat/KSRatio/$next_working_day
tmp_ksratio="/tmp/agree_tmp_details_lookup_adjust"
mkdir /spare/local/MidtermStrat/KSRatio
echo "KS Ratio File Updating $ksOut"
cp $ksOut ${ksOut}_Before_adjustment_bkp
>$tmp_ksratio

#Update FUT0 to FUT1
egrep "SIGMA_BETA_|ASSET_" $ksOut | sed 's/_FUT0/_FUT1/g' >$tmp_ksratio

#Update Closing Price
sym_id_=`/home/pengine/prod/live_execs/get_exchange_symbol NSE_SBIN_FUT1 $date`
expiry_date_=`grep $sym_id_ /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt | cut -d'_' -f4`
expiry_date_format_=`date -d"$expiry_date_" +%d-%b-%Y`
DD=${date:6:2}
MM=${date:4:2}
YY=${date:2:2}
YYYY=${date:0:4}
MSTR=$(echo $(date -d $date +%b) | awk '{print toupper($1)}') ;
bhavcopy_path="/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/fo"$DD"$MSTR"$YYYY"bhav.csv"
echo "BhavCopy: $bhavcopy_path"
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
      exit;
fi
cat $bhavcopy_path | grep $expiry_date_format_ | grep FUT | awk -F',' '{print "AGGRE_NSE_"$2"_FUT1",$10}' >/tmp/symbol_settle_price_size_69
while IFS=' ' read -r key value; do
    symbol_settle[$key]=$value
done < /tmp/symbol_settle_price_size_69

while IFS=',' read -r f1 f2; do
#  echo "F1: $f1"
  sym=`echo $f1 | cut -d' ' -f1`
#  echo $sym
  echo "$f1,${symbol_settle[$sym]}"
  echo "$f1,${symbol_settle[$sym]}" >>$tmp_ksratio
done < <(cat $ksOut | grep "AGGRE_NSE_" | sed 's/_FUT0/_FUT1/g')

tmp_product_="/tmp/product_details_for_adjustment"

grep "ALGO_" $ksOut  | cut -d'_' -f3 | sort | uniq >$tmp_product_
day_before_expirydate_=$date
echo "DATE: $day_before_expirydate_"

while IFS='' read -r line || [[ -n "$line" ]]; 
do
  ratio=$(grep $day_before_expirydate_ "/spare/local/NseHftFiles/Ratio/EndRatio/NSE_"$line"_FUT1_NSE_"$line"_FUT0" | awk '{print $2}');
  if [[ "$ratio" != "" ]]; then
    echo "$line - $ratio enter"
    grep ALGO_NSE_"$line" $ksOut | awk -F',' '{OFS = "," ; print $1,$2,$3*'$ratio',$4/'$ratio',$5,$6,$7,$8,$9}' | sed 's/_FUT0/_FUT1/g' >>$tmp_ksratio
  else
    echo "$line - $ratio - RATIO 0"
    grep ALGO_NSE_"$line" $ksOut | sed 's/_FUT0/_FUT1/g' >>$tmp_ksratio
  fi;
done < $tmp_product_

#copy the changes back to Ksratio file
cp $tmp_ksratio $ksOut
rsync -avz /spare/local/MidtermStrat/ 52.90.0.239:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.64:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.65:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.69:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.84:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.72:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.63:/spare/local/MidtermStrat

echo "Check Contract Diff Mail For Any Lot Size Update" | mailx -s "KsRatio Daily File Adjuted for FUT0: ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in



