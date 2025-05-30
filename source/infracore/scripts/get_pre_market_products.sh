#!/bin/bash

i=0;
date=`date -d "1 day ago" +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
while [ $is_holiday = "1" ] ;
do
   i=$(( i + 1))
#   echo "i= $i, date-> $date "
   date=`date -d "$i day ago" +\%Y\%m\%d`
   is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
#   echo "NSE Holiday. Exiting...";
      #exit;
done


echo "date:: $date"
ssh dvcinfra@10.23.5.13 "less /NAS1/data/MFGlobalTrades/ind_pnls/CM/eod_pnls/ind_pnl_$date'.txt' | grep -v 'TOTAL_POS:      0 ' | awk '{if(\$26 > 0) print \$31,-\$26}'" > /home/dvctrader/ATHENA/pos_exec_file_temp

/home/dvctrader/ATHENA/pre_market_price.sh $date

true>/home/dvctrader/ATHENA/pre_market_asm_products ;

for prod in `cat /home/dvctrader/ATHENA/pos_exec_file_temp | awk '{print $1}'` ;
do
  echo $prod
  if [ `grep $prod /home/pengine/prod/live_configs/sdv-ind-srv17_addts.cfg | wc -l` -eq 0 ]; then
    echo "$prod">> /home/dvctrader/ATHENA/pre_market_asm_products 
  fi
done

