#!/bin/bash
work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details_CM/'
mail_file_cm="/tmp/mail_cm_fileors"
tmp_file1="/tmp/fileunsortcmors"
tmp_file4="/tmp/fileorssymratiofo"
error_file_check="/tmp/error_ors_report_cm"
declare -A server_to_ip_map
server_to_ip_map=( ["IND16"]='10.23.227.81' ["IND17"]='10.23.227.82' ["IND18"]='10.23.227.83' ["IND23"]='10.23.227.72' ["IND24"]='10.23.227.74' )


declare -A month_product_cm
declare -A month_exec_cm
declare -A month_product_price_cm
declare -A month_exec_price_cm
declare -A month_modifyof_cm
declare -A month_modify_cm
declare -A month_mds_msg_cm
declare -A month_trade_cm
declare -A month_mds_modify_cm
declare -A month_mds_modify_price_cm
declare -A product_price

SendMail(){
      (echo To: "raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "Daily ORS Report for CASH Products $today_"; echo "Content-Type: text/html;";cat ${mail_file_cm}) | /usr/sbin/sendmail -t
#      (echo To: "subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "Daily ORS Report for CASH Products $today_"; echo "Content-Type: text/html;";cat ${mail_file_cm}) | /usr/sbin/sendmail -t
}

ErrorMail(){
      (echo To: "raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for CASH Products $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
#      (echo To: "subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for CASH Products $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
        exit;
}

GetPreviousWorkingDay() {
    prev_day=`/home/pengine/prod/live_execs/update_date $prev_day P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
    while [ $is_holiday_ = "1" ];
    do
         prev_day=`/home/pengine/prod/live_execs/update_date $prev_day P A`
         is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
    done
}

while true
do
  [[ `ps -aux | grep "get_order_ors_cm" | grep -v grep | wc -l` == "0" ]] && break
  echo "get_order_ors_cm running"
  sleep 5m
done
[[ -f $error_file_check ]] && echo "ERROR EXIT" && exit

if [ $# -ne 1 ] ; then
    echo "Called As : " $* ;
    echo "$0 Date todays" ;
    exit ;
fi

today_=$1
prev_day=$1
GetPreviousWorkingDay ;

YYYY=${prev_day:0:4}
MM=${prev_day:4:2}
DD=${prev_day:6:2}
TODAY_MM=${today_:4:2}
MSTR=$(echo $(date -d $today_ +%b) | awk '{print toupper($1)}') ;
count=0

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
exit;
fi

if [ ! -f $work_dir/${today_}_cm ]; then
    echo "$work_dir/${today_}_cm file not present"
    (echo To: "raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "ORS Report CASH Products File Not Present ${today_}";) | /usr/sbin/sendmail -t
    exit;
fi

echo "$work_dir/${today_}_cm"
while IFS=' ' read -r sym_t f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 f20 f21 f22 f23 f24 f25 f26 f27 f28 f29 f30 f31 f32
do
    echo "$sym_t $f1 $f2 $f3 $f4 $f5 $f6 $f7 $f8 $f9 $f10 $f11 $f12 $f13 $f14 $f15 $f16 $f17 $f18 $f19 $f20 $f21 $f22 $f23 $f24 $f25 $f26 $f27 $f28 $f29 $f30 $f31 $f32"
    product_count=$(( product_count + 1 ))
    if [ ${month_product_cm[$sym_t]} ]; then
         month_modify_cm[$sym_t]=$(( ${month_modify_cm[$sym_t]} + $f21))
         month_modifyof_cm[$sym_t]=$(( ${month_modifyof_cm[$sym_t]} + $f22))
         month_product_cm[$sym_t]=$(( ${month_product_cm[$sym_t]} + $f23))
         month_exec_cm[$sym_t]=$(( ${month_exec_cm[$sym_t]} + $f24))
         month_product_price_cm[$sym_t]=$(echo "${month_product_price_cm[$sym_t]} + $f25" | bc)
         month_exec_price_cm[$sym_t]=$(echo "${month_exec_price_cm[$sym_t]} + $f26" | bc)
         month_mds_msg_cm[$sym_t]=$(( ${month_mds_msg_cm[$sym_t]} + $f28))
         month_trade_cm[$sym_t]=$(( ${month_trade_cm[$sym_t]} + $f29))
         month_mds_modify_cm[$sym_t]=$(( ${month_mds_modify_cm[$sym_t]} + $f31))
         month_mds_modify_price_cm[$sym_t]=$(( ${month_mds_modify_price_cm[$sym_t]} + $f32))
    else
         month_modify_cm[$sym_t]=$f21
         month_modifyof_cm[$sym_t]=$f22
         month_product_cm[$sym_t]=$f23
         month_exec_cm[$sym_t]=$f24
         month_product_price_cm[$sym_t]=$f25
         month_exec_price_cm[$sym_t]=$f26
         month_mds_msg_cm[$sym_t]=$f28
         month_trade_cm[$sym_t]=$f29
         month_mds_modify_cm[$sym_t]=$f31
         month_mds_modify_price_cm[$sym_t]=$f32
    fi
done < $work_dir/${today_}_cm
echo "$work_dir/${today_}_cm file read"
#done

>$tmp_file1
>$tmp_file4

for sym in "${!month_product_cm[@]}"; do
    ratio_order=${month_product_cm[$sym]};
    ratio_order_price=${month_product_price_cm[$sym]};
    percent_modify=0
    ratio_trade=${month_mds_msg_cm[$sym]};
    [[ ${month_exec_cm[$sym]} == 0 ]] || ratio_order=$(echo "${month_product_cm[$sym]} / ${month_exec_cm[$sym]}" | bc )
    [[ ${month_exec_price_cm[$sym]} == "0" ]] || [[ ${month_exec_price_cm[$sym]} == "0.00" ]] || ratio_order_price=$(echo "${month_product_price_cm[$sym]} / ${month_exec_price_cm[$sym]}" | bc )
    [[ ${month_modifyof_cm[$sym]} == 0 ]] || percent_modify=$(echo "${month_modifyof_cm[$sym]} * 100 / ${month_modify_cm[$sym]}" | bc)
    [[ ${month_trade_cm[$sym]} == 0 ]] || ratio_trade=$(echo "${month_mds_msg_cm[$sym]} / ${month_trade_cm[$sym]}" | bc)
#    [[ ! ${product_price[$sym]+abc} ]] && { echo "Not exists $sym"; product_price[$sym]=0; }
#      if [[ ${month_product_cm[$sym]} -gt 100000 ]]; then
        echo "$sym ${month_product_cm[$sym]} ${month_exec_cm[$sym]} $ratio_order ${month_product_price_cm[$sym]} ${month_exec_price_cm[$sym]} $ratio_order_price ${month_modify_cm[$sym]} ${month_modifyof_cm[$sym]} $percent_modify ${month_mds_msg_cm[$sym]} ${month_trade_cm[$sym]} $ratio_trade ${month_mds_modify_cm[$sym]} ${month_mds_modify_price_cm[$sym]}" >> $tmp_file1
#      fi                                                                                
done
echo "ratio computation done"

#[[ $product_count != `wc -l $tmp_file1 | cut -d' ' -f1` ]] && ErrorMail 
sort -n -rk 4 $tmp_file1 > $tmp_file4
echo "sorting done"

total_modify_cm=0; total_modifyof_cm=0; total_product_cm=0; total_exec_cm=0; total_product_price_cm=0; total_exec_price_cm=0; 
total_mds_msg_cm=0; total_trade_cm=0; total_mds_modify_cm=0; total_mds_modify_price_cm=0;

while IFS=' ' read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14
do
      product_count_cm=$(( product_count_cm + 1 ))
      total_modify_cm=$(echo "$f7 + $total_modify_cm" | bc); total_modifyof_cm=$(echo "$f8 + $total_modifyof_cm" | bc);
      total_product_cm=$(echo "$f1 + $total_product_cm" | bc); total_exec_cm=$(echo "$f2 + $total_exec_cm" | bc);
      total_product_price_cm=$(echo "$f4 + $total_product_price_cm" | bc); total_exec_price_cm=$(echo "$f5 + $total_exec_price_cm" | bc);
      total_mds_msg_cm=$(echo "$f10 + $total_mds_msg_cm" | bc); total_trade_cm=$(echo "$f11 + $total_trade_cm" | bc);
      total_mds_modify_cm=$(echo "$f13 + $total_mds_modify_cm" | bc);
      total_mds_modify_price_cm=$(echo "$f14 + $total_mds_modify_price_cm" | bc);
done < $tmp_file4

total_ratio_order_cm=$total_product_cm; total_ratio_order_price_cm=$total_product_price_cm; total_percent_modify_cm=0; total_ratio_trade_cm=$total_mds_msg_cm;
[[ ${total_exec_cm} == 0 ]] || total_ratio_order_cm=$(echo "${total_product_cm} / ${total_exec_cm}" | bc)
[[ ${total_exec_price_cm} == "0" ]] || [[ ${total_exec_price_cm} == "0.00" ]] || total_ratio_order_price_cm=$(echo "${total_product_price_cm} / ${total_exec_price_cm}" | bc)
[[ ${total_modifyof_cm} == 0 ]] || total_percent_modify_cm=$(echo "${total_modifyof_cm} * 100 / ${total_modify_cm}" | bc)
[[ ${total_trade_cm} == 0 ]] || total_ratio_trade_cm=$(echo "${total_mds_msg_cm} / ${total_trade_cm}" | bc)

echo "total ratio computation done"

>$mail_file_cm
echo -e "<p>$today_ Product Count: $product_count_cm</p>" >> $mail_file_cm

echo -e "<br/><table border="1"><thead><th> Products </th><th> Total </th><th> Exec </th><th> Ratio </th><th> Total_Price </th><th> Exec_Price </th><th> Ratio_Price </th><th> Modify </th><th> ModifyOF </th><th> ModifyOf% </th><th> Total_MDS_MSG </th><th> Total_Trade </th><th> Trade_Ratio </th><th> Total_MDS_Modify </th><th> Total_MDS_Modify_Price </th><th> Total_MDS_Modify_Price_OF% </th><tbody>" >> $mail_file_cm

modify_of_to_modify_price=$(echo "${total_modifyof_cm} * 100 / ${total_mds_modify_price_cm}" | bc)
echo "<tr><td>OVERALL</td><td>$total_product_cm</td><td>$total_exec_cm</td><td>$total_ratio_order_cm</td><td>$total_product_price_cm</td><td>$total_exec_price_cm</td><td>$total_ratio_order_price_cm</td><td>$total_modify_cm</td><td>$total_modifyof_cm</td><td>$total_percent_modify_cm</td><td>$total_mds_msg_cm</td><td>$total_trade_cm</td><td>$total_ratio_trade_cm</td><td>$total_mds_modify_cm</td><td>$total_mds_modify_price_cm</td><td>$modify_of_to_modify_price</td></tr>" >> $mail_file_cm

while IFS=' ' read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14
do
    modify_of_to_modify_price=$(echo "${f8} * 100 / ${f14}" | bc)
      echo "<tr><td>$sym</td><td>$f1</td><td>$f2</td><td>$f3</td><td>$f4</td><td>$f5</td><td>$f6</td><td>$f7</td><td>$f8</td><td>$f9</td><td>$f10</td><td>$f11</td><td>$f12</td><td>$f13</td><td>$f14</td><td>$modify_of_to_modify_price</td></tr>" >> $mail_file_cm
done < $tmp_file4

echo -e "</tbody></table>" >> $mail_file_cm
echo -e "<br/><br/>" >> $mail_file_cm

echo "table added"

SendMail
echo "mail sent"
echo "generating html ors report for cm"

/home/dvcinfra/important/Generate_ors_detail/OrsReportCM/generate_ors_report_cm.sh $today_

echo "generated html ors report for cm"

