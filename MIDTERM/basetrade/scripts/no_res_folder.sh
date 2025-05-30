$HOME/basetrade/scripts/dump_and_sync_results.sh N ALL 10.0.1.15 3  &>/dev/null

while read line; do
shortcode=`echo $line | cut -d' ' -f1`
time_period=`echo $line | cut -d' ' -f2`
is_valid=`perl ~/basetrade/scripts/folder.pl $time_period`
#echo "$is_valid";
if [ "$is_valid" != "1" ]; then
  continue;
fi

yyyy=`date +%Y`
mm=`date +%m`
dd=`date +%d`

is_holiday=`/home/dvctrader/basetrade/scripts/is_product_holiday.pl $yyyy$mm$dd $shortcode`
if [ "$is_holiday" == "1" ]; then
  continue
fi

find  /home/dvctrader/modelling/strats/$shortcode/$time_period/ -type f | awk -F'/' '{print $NF}' > rlog
total=`cat rlog | wc -l`

if [ ! -f /home/dvctrader/ec2_globalresults/$shortcode/$yyyy/$mm/$dd/results_database.txt ]; then
	echo $shortcode $time_period "Total: "$total "No results: "$total ", Zero results: 0"
	continue;
fi

cat /home/dvctrader/ec2_globalresults/$shortcode/$yyyy/$mm/$dd/results_database.txt  | cut -d' ' -f1 &> rlog2

output=`comm -13 <( sort rlog2 ) <( sort rlog ) | wc -l`
rcount=`/home/dvctrader/basetrade_install/bin/summarize_strategy_results $shortcode /home/dvctrader/modelling/strats/$shortcode/$time_period/ DB $yyyy$mm$dd $yyyy$mm$dd INVALIDFILE kCNAPnlSharpeAverage 0 IF 0 | grep $yyyy$mm$dd | wc -l`
zero_results=`expr $total - $rcount`
zero_results=`expr $zero_results - $output`
if [[ ( $output -gt 0 ) || ( $zero_results -gt 5 ) ]]; then
  echo $shortcode $time_period "Total: "$total "No results: "$output ", Zero results: " $zero_results
fi
done < /spare/local/files/ALL-timeperiod.txt
