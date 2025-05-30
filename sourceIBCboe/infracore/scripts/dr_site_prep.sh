#!/bin/bash

mock_output_file="/tmp/dr_site_test_mock_output_file_"
product_mdslog_file_dir="/spare/local/ORSBCAST_MULTISHM/NSE"
product_tmp_mdslogreader_file="/tmp/tmp_product_mdslogreader_file_for_test_mock"
cs_ee_prod_file="/tmp/tes_mock_prod_cs_ee_value_latency"
saos_file="/tmp/test_dr_latency_saos_file"
date=`date +"%Y%m%d"`
cme_instance=`ps aux| grep cme | grep -v grep | awk '{print $11}' |wc -l`

if [[ $cme_instance -eq 0 ]]; then
   echo "no instance of ors running"
   exit
fi 
>$cs_ee_prod_file

/home/pengine/prod/live_execs/console_trader_dr_check >"$mock_output_file"
sleep 5s

log_file=`grep -a "Printing" "$mock_output_file" | awk '{print $6}'`
grep "shortcode" "$log_file" | grep "SE:" |awk '{print $10}' >$saos_file
stock=`grep "shortcode" "$log_file" | grep "SE:" |awk '{print $4}' | uniq`
echo "stock $stock logfile $log_file"
   search_string=`cat $saos_file |xargs | tr ' ' '|'`

   echo "searchstring $search_string"
   ssh dvcinfra@10.23.227.63 "LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1  /home/pengine/prod/live_execs/mds_log_reader ORS_REPLY_LIVE ${product_mdslog_file_dir}/${stock}_$date|egrep '$search_string' | grep -w Rejc |grep 'SE: 5' " | awk '{print $31" "$32" "$43" "$44" "($44-$32)/3600}' >>"$cs_ee_prod_file"

mean=`awk '{ sum += $5 } END {if (NR > 0) print sum / NR}' $cs_ee_prod_file`
echo "mean :: $mean";

if /apps/anaconda/anaconda3/bin/python3 -c "exit(0 if $mean > 200 else 1)" ;then
   echo "Exchange latency: $mean"|mailx -s "Increased exchange latency (DR Site)" -r "${HOSTNAME}-${USER}<naman.jain@tworoads-trading.co.in>" naman.jain@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in;
else
   echo "Exchange latency: $mean"|mailx -s "decreased exchange latency (non DR Site)" -r "${HOSTNAME}-${USER}<naman.jain@tworoads-trading.co.in>" naman.jain@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in;
fi


