#!/bin/bash

if [ $# -ne 2 ]; then
  echo "USAGE: <SCRIPT> <TRADE_FILE> <YYYYMMDD>"
  exit
fi

trade_file=$1;
date_=$2;
fo_eod_pnl_file="/NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${date_}.txt"
mail_result="/tmp/strat_pnl_mail_file.txt"
>$mail_result

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

echo '<div style="display:flex;justify-content:space-around">' >>$mail_result
echo "<div style='margin:0 30px;'>" >>$mail_result
echo "<h3>LogFile: $trade_file</h3>" >>$mail_result
echo "<table border='1' id='myTable' class='table table-striped' style='border-collapse: collapse'<thead><tr><th>PRODUCT</th><th>LOG_PNL</th><th>ADJUST_PNL</th><th>PNL_DIFF</th</tr></thead><tbody>" >> $mail_result

total_log_pnl=0; total_adjust_pnl=0; total_pnl_diff=0;
for prod in `zgrep "SIMRESULT NSE_" $trade_file | grep -v "POS: 0" | cut -d' ' -f3 | sort | uniq`;
do
  shortcode=`echo "$prod" | cut -d'_' -f2,3`
  set_price=`ssh dvcinfra@10.23.5.26 "grep -w $shortcode $fo_eod_pnl_file" | awk '{print $20}'`

  pnl_output=`zgrep "SIMRESULT $prod" $trade_file | sort -n -k1 | awk -F'[][]' '{print $1,$(NF-1)}' | awk '{print $5,$7,$(NF),$(NF-2)}' | awk -v setprice="${set_price}" '{ if (($3 > 0) && ($4 > 0)) { price=($3 + $4)/2; log_pnl=$1; adjust_pnl=($1 + ($2 * (setprice-price))); pnl_diff=($2 * (setprice-price)); }} END {printf"%.2f %.2f %.2f", log_pnl,adjust_pnl,pnl_diff;}'`

  log_pnl=`echo $pnl_output | cut -d' ' -f1`
  total_log_pnl=$(echo "$log_pnl + $total_log_pnl" | bc )
  adjust_pnl=`echo $pnl_output | cut -d' ' -f2`
  total_adjust_pnl=$(echo "$adjust_pnl + $total_adjust_pnl" | bc )
  pnl_diff=`echo $pnl_output | cut -d' ' -f3`
  total_pnl_diff=$(echo "$pnl_diff + $total_pnl_diff" | bc )
  echo -e "<tr><td>$shortcode</td><td>$log_pnl</td><td>$adjust_pnl</td><td>$pnl_diff</td></tr>" >> $mail_result
  echo "$shortcode $log_pnl $adjust_pnl $pnl_diff" 
done

echo -e "<tr><td>TOTAL</td><td>$total_log_pnl</td><td>$total_adjust_pnl</td><td>$total_pnl_diff</td></tr>" >> $mail_result
echo "TOTAL $total_log_pnl $total_adjust_pnl $total_pnl_diff" 
echo "</tbody></table>" >> $mail_result
echo "</div></body></html>" >> $mail_result

(echo To: "subham.chawda@tworoads-trading.co.in nseall@tworoads.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "IntraDay PNL Adjust $date_"; echo "Content-Type: text/html;";cat ${mail_result}) | /usr/sbin/sendmail -t
#(echo To: "subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "Inter Day PNL Adjust $date_"; echo "Content-Type: text/html;";cat ${mail_result}) | /usr/sbin/sendmail -t


