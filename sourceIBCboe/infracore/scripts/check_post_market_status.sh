#!/bin/bash
loggin_file="/tmp/mail_run_check_for_post_mkt_file"

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}


date_=`date +"%Y%m%d"`
YYYYMMDD=$date_

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

if [[ $1 == "PRE" ]]; then
        GetPreviousWorkingDay
fi

echo "DATE: $date_ FILE_DATE: $YYYYMMDD"


if [[ $1 == "PRE" ]]; then
        scp dvctrader@10.23.227.82:/home/dvctrader/ATHENA/pos_exec_file_temp /tmp/product_IND17_to_post_makrt_${date_}
        cat /tmp/product_IND17_to_post_makrt_${date_} | cut -d' ' -f1,2 >/tmp/tmp_file_gener_mkt
        mv /tmp/tmp_file_gener_mkt /tmp/product_IND17_to_post_makrt_${date_}
# IGNORE SHORT POS as it goes in auction
        cat /NAS1/data/MFGlobalTrades/ind_pnls/CM/eod_pnls/ind_pnl_${YYYYMMDD}.txt | grep -v "TOTAL_POS:      0" | grep NSE_ |  awk -F'|' '{print $10, $12}' | awk '{if ($2>0){print $3,-$2}}' | grep -f /spare/local/tradeinfo/NSE_Files/SecuritiesAvailableForTrading/sec_list.csv_${YYYYMMDD} | sort >/tmp/product_to_post_makrt_${date_}
        if [ -s /tmp/product_to_post_makrt_${date_} ]; then
          awk '{print $1" 10000"}' /tmp/product_to_post_makrt_${date_} > /tmp/addts_product_premarket_${date_}
          scp /tmp/addts_product_premarket_${date_} dvcinfra@10.23.227.82:/tmp/addts_product_premarket_${date_}
          ssh dvcinfra@10.23.227.82 "/home/pengine/prod/live_scripts/orsRejectHandling.sh addts /tmp/addts_product_premarket_${date_}"
        fi
else
        
        cat /NAS1/data/MFGlobalTrades/ind_pnls/CM/eod_pnls/ind_pnl_${YYYYMMDD}.txt | grep -v "TOTAL_POS:      0" | grep NSE_ |  awk -F'|' '{print $10, $12}' | awk '{print $3,-$2}' | grep -f /spare/local/tradeinfo/NSE_Files/SecuritiesAvailableForTrading/sec_list.csv_${YYYYMMDD} | sort >/tmp/product_to_post_makrt_${date_}
        scp dvctrader@10.23.227.82:/home/dvctrader/ATHENA/pos_to_exit_${date_} /tmp/product_IND17_to_post_makrt_${date_}
        
fi
running_status_=`ssh dvctrader@10.23.227.82 "ps aux | grep post_market_exec | grep -v grep | wc -l"`

if [ ! -s /tmp/product_to_post_makrt_${date_} ]; then
        echo "" |mailx -s "No Product For Post Mrkt RunningStatus: $running_status_" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
        exit
fi
 
sort /tmp/product_IND17_to_post_makrt_${date_} >/tmp/pos_mkt_to_close_tmp_file
mv /tmp/pos_mkt_to_close_tmp_file /tmp/product_IND17_to_post_makrt_${date_}

diff /tmp/product_IND17_to_post_makrt_${date_} /tmp/product_to_post_makrt_${date_} >/tmp/post_check_file_diff

id_=`ssh dvctrader@10.23.227.82 ps aux | grep post_market_exec | grep -v grep | awk '{print $16}'`
echo "ID: $id_"
check_=`ssh dvctrader@10.23.227.82 "grep 'All set going into sleep now' /spare/local/logs/tradelogs/log.${date_}.${id_} | wc -l"`

count_t=`wc -l /tmp/product_IND17_to_post_makrt_${date_} | cut -d' ' -f1 `
echo "No of Products:     $count_t" >$loggin_file 
echo "PostMarket Status:  $running_status_ ID: $id_" >>$loggin_file
echo "ForceAllIndicator:  $check_" >>$loggin_file



if [ -s /tmp/post_check_file_diff ]
then
   echo "DIFF: " >>$loggin_file
   comm -3 /tmp/product_IND17_to_post_makrt_${date_} /tmp/product_to_post_makrt_${date_} >>$loggin_file 
   cat $loggin_file | mailx -s "Error Post Mrkt Product Diff" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
#subham.chawda@tworoads-trading.co.in
else
   echo "Product: " >>$loggin_file
   cat /tmp/product_to_post_makrt_${date_} >>$loggin_file
   cat  $loggin_file | mailx -s "Post Mkt on IND17" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
fi

