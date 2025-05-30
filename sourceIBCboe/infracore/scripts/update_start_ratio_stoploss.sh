#!/bin/bash
[ $# -ne 1 ] && { echo "DATE ARGUMENT REQUIRED"; exit; }
today=$1
mail_file=/tmp/stoploss_update_mail_${today}
>${mail_file}
prev_tmp=`date -d "yesterday" +"%Y-%m-%d"`

cd /spare/local/files/NSE/PreMarketOpenRatio/
if [ ! -f $today ];then
	exit -1
fi

prev_day=`/home/pengine/prod/live_execs/update_date ${today} P A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ${prev_day} T`
while [ $is_holiday = "1" ];
do
  prev_day=`/home/pengine/prod/live_execs/update_date ${prev_day} P A`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ${prev_day} T`
done
date_to_grep=${prev_day:0:4}"-"${prev_day:4:2}"-"${prev_day:6:2};

live_file=`cat /home/dvctrader/ATHENA/run.sh | grep START_RATIO | grep -v "^#" | awk '{print $2}'`
trader_id=`cat /home/dvctrader/ATHENA/run.sh | grep START_RATIO | grep -v "^#" | awk '{print $6}'`
start_ratio_dir=`echo $live_file | awk  -F "/" '{print $1"/"$2"/"$3"/"$4"/"$5}'`
start_ratio_products=`cat ${live_file} |  grep THEO |awk -F "=" '{print $2}' | awk -F "_" '{print $2}' | xargs | tr ' ' '|'`

products_to_adjust='/tmp/products_adjust_stoploss'
old_stop_loss=${start_ratio_dir}'/old_stoploss.'${today}
cat ${today} | awk '{if (($2>=2 || $2 <= -2)&& ($2<=9 && $2 >=-9)){print $1,$2}}' | sed -r 's/-([0-9]+\.[0-9]+)/\1-/g;' | sort -k2n | sed -r 's/([0-9]+\.[0-9]+)-/-\1/g;' | tail -25 | egrep -w "${start_ratio_products}" > ${products_to_adjust}
>${old_stop_loss}

echo $date_to_grep
for prod in `egrep "${date_to_grep}" /spare/local/tradeinfo/NSE_Files/EarningsReports/consolidated_earnings.csv | awk '{print $1}'`;
do
        if ! grep -q $prod ${products_to_adjust};then
	                grep "${prod}" ${today} >> ${products_to_adjust}
        fi
done


cd ${start_ratio_dir}
echo "Dir ${start_ratio_dir}"
new_stop_loss_val=25000
for prod in `cat ${products_to_adjust} | awk '{print $1}'`;
do
	old_value=`grep -w STOP_LOSS "NSE_${prod}_MM/MainConfig.cfg" | awk '{gsub(" ", "", $3);print $3}'`;
	echo "$prod" ${old_value} >> ${old_stop_loss}
	sed -i "s/\<STOP_LOSS = .*\>/STOP_LOSS = ${new_stop_loss_val}/g" "NSE_${prod}_MM/MainConfig.cfg"
done
echo "${old_stop_loss}"
for prod in `cat ${products_to_adjust} | awk '{print $1}'`;
do 
  old_value=`grep -w "${prod}" ${old_stop_loss} | awk '{print $2}'`
  new_value=`grep -w STOP_LOSS "NSE_${prod}_MM/MainConfig.cfg" | awk '{print $3}'`
  echo $prod" "$old_value" "$new_value >> ${mail_file}
done

/home/pengine/prod/live_execs/user_msg --traderid $trader_id --showindicators

rm -rf $products_to_adjust
