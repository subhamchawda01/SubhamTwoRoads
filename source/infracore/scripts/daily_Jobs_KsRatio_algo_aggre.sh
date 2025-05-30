#!/bin/bash
tmp_ksratio="/tmp/agree_tmp_details_lookup"

declare -A dict
declare -A symbol_settle

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 YYYYMMDD" ;
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


id="123469"
sym_id_=`/home/pengine/prod/live_execs/get_exchange_symbol NSE_SBIN_FUT0 $date`
expiry_date_=`grep $sym_id_ /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt | cut -d'_' -f4`
expiry_date_format_=`date -d"$expiry_date_" +%d-%b-%Y`
strat_log="/spare/local/logs/tradelogs/log.${date}.${id}.gz"
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
cat $bhavcopy_path | grep $expiry_date_format_ | grep FUT | awk -F',' '{print "AGGRE_NSE_"$2"_FUT0",$10}' >/tmp/symbol_settle_price_size_69
while IFS=' ' read -r key value; do
    symbol_settle[$key]=$value
done < /tmp/symbol_settle_price_size_69


ksOut=/spare/local/MidtermStrat/KSRatio/$next_working_day
mkdir /spare/local/MidtermStrat/KSRatio
echo "KS Ratio File Updating $ksOut"

echo "StratFIle: $strat_log"
ssh 10.23.227.64 "zgrep AGGRE_ $strat_log" | awk -F ' ' '{ h[$1] = $3; next} END { for (i in h) {print i,"=",h[i]}}' | grep -v " = 0" | tr ',' ' ' | sed 's/_FUT1/_FUT0/g' > $tmp_ksratio
while IFS=' ' read -r f1 f2 f3 f4; do
  echo "$f1 = $f3,${symbol_settle[$f1]}" 
  echo "$f1 = $f3,${symbol_settle[$f1]}" >>$ksOut
done < $tmp_ksratio



ssh 10.23.227.64 "zgrep ALGO_NSE_ $strat_log"  |  grep = | awk -F ' ' '{ h[$1] = $0; next} END { for (i in h) {print h[i]}}' | sed 's/_FUT1/_FUT0/g' >>$ksOut


count=`grep ALGO_ $ksOut | wc -l`
count2=`grep ASSET_LONG $ksOut | wc -l`
count3=`grep AGGRE_ $ksOut | wc -l`
echo "Algo Count: $count Long: $count2 AGGRE: $count3"
if [[ $count -lt 1 || $count2 -ne 1 || $count3 -lt 1 ]] ; then
    echo "Algo Count Missing "
  echo "Algo Count: $count Long: $count2 AGGRE: $count3" | mailx -s "Algo Count Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit
fi
rsync -avz /spare/local/MidtermStrat/ 52.90.0.239:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.64:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.65:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.69:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.84:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.72:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.63:/spare/local/MidtermStrat


echo "" | mailx -s "KsRatio Daily File Updated: ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in

