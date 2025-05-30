#!/bin/bash
work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details_FO/'
mail_file="/tmp/mail_fo_fileors"
tmp_file1="/tmp/fileunsortfoors"
tmp_file2="/tmp/filesortfoors"
tmp_file3="/tmp/filesort10exchsym"
tmp_file4="/tmp/fileorssymratiofo"
datasource="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
exec_exchsym_to_nsesym="/home/pengine/prod/live_execs/get_shortcode_from_ds"
error_file_check="/tmp/error_ors_report_fo"
declare -A server_to_ip_map
server_to_ip_map=( ["IND14"]='10.23.227.64' ["IND15"]='10.23.227.65' ["IND19"]='10.23.227.69' ["IND20"]='10.23.227.84' )


declare -A month_product_fo
declare -A month_exec_fo
declare -A month_modifyof_fo
declare -A month_modify_fo
declare -A month_mds_msg_fo
declare -A month_trade_fo
declare -A month_mds_modify_fo
declare -A product_price

SendMail(){
#      (echo To: "raghunandan.sharma@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"; echo Subject: "Daily ORS Report for FO Products $date_"; echo "Content-Type: text/html;";cat ${mail_file}) | /usr/sbin/sendmail -t
      (echo To: "raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "Daily ORS Report for FO Products $today_"; echo "Content-Type: text/html;";cat ${mail_file}) | /usr/sbin/sendmail -t
#      (echo To: "subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "Daily ORS Report for FO Products $today_"; echo "Content-Type: text/html;";cat ${mail_file}) | /usr/sbin/sendmail -t
}

ErrorMail(){
#      (echo To: "raghunandan.sharma@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for FO Products $date_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
#      (echo To: "raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for FO Products $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
      (echo To: "subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for FO Products $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
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
  [[ `ps -aux | grep "get_order_ors_fo" | grep -v grep | wc -l` == "0" ]] && break
  echo "get_order_ors_fo running"
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

if [ ! -f $work_dir/${today_}_fo ]; then
    echo "$work_dir/${today_}_fo file not present"
    (echo To: "raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "ORS Report FO Products File Not Present ${today_}";) | /usr/sbin/sendmail -t
    exit;
fi

#for foFile in `ls -t $work_dir/* | head -20 | grep "_*fo"`;
#do
echo "$work_dir/${today_}_fo"
while IFS=' ' read -r sym_t f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 f20 f21 f22 f23 f24 f25
do
    echo "$sym_t $f1 $f2 $f3 $f4 $f5 $f6 $f7 $f8 $f9 $f10 $f11 $f12 $f13 $f14 $f15 $f16 $f17 $f18 $f19 $f20 $f21 $f22 $f23 $f24 $f25"
    product_count=$(( product_count + 1 ))
    if [ ${month_product_fo[$sym_t]} ]; then
         month_modify_fo[$sym_t]=$(( ${month_modify_fo[$sym_t]} + $f17))
         month_modifyof_fo[$sym_t]=$(( ${month_modifyof_fo[$sym_t]} + $f18))
         month_product_fo[$sym_t]=$(( ${month_product_fo[$sym_t]} + $f19))
         month_exec_fo[$sym_t]=$(( ${month_exec_fo[$sym_t]} +  $f20))
         month_mds_msg_fo[$sym_t]=$(( ${month_mds_msg_fo[$sym_t]} +  $f22))
         month_trade_fo[$sym_t]=$(( ${month_trade_fo[$sym_t]} +  $f23))
         month_mds_modify_fo[$sym_t]=$(( ${month_mds_modify_fo[$sym_t]} +  $f25))
    else
         month_modify_fo[$sym_t]=$f17
         month_modifyof_fo[$sym_t]=$f18
         month_product_fo[$sym_t]=$f19
         month_exec_fo[$sym_t]=$f20
         month_mds_msg_fo[$sym_t]=$f22
         month_trade_fo[$sym_t]=$f23
         month_mds_modify_fo[$sym_t]=$f25
    fi
done < $work_dir/${today_}_fo
echo "$work_dir/${today_}_fo file read"
#done

>$tmp_file1
>$tmp_file2
>$tmp_file3
>$tmp_file4

for sym in "${!month_product_fo[@]}"; do
    ratio_order=${month_product_fo[$sym]};
    percent_modify=0
    ratio_trade=${month_mds_msg_fo[$sym]};
    [[ ${month_exec_fo[$sym]} == 0 ]] || ratio_order=$(( ${month_product_fo[$sym]} / ${month_exec_fo[$sym]} ))
    [[ ${month_modifyof_fo[$sym]} == 0 ]] || percent_modify=$(( ${month_modifyof_fo[$sym]} * 100 / ${month_modify_fo[$sym]} ))
    [[ ${month_trade_fo[$sym]} == 0 ]] || ratio_trade=$(( ${month_mds_msg_fo[$sym]} / ${month_trade_fo[$sym]} ))
#    [[ ! ${product_price[$sym]+abc} ]] && { echo "Not exists $sym"; product_price[$sym]=0; }
#      if [[ ${month_product_fo[$sym]} -gt 100000 ]]; then
        echo "$sym ${month_product_fo[$sym]} ${month_exec_fo[$sym]} $ratio_order ${month_modify_fo[$sym]} ${month_modifyof_fo[$sym]} $percent_modify ${month_mds_msg_fo[$sym]} ${month_trade_fo[$sym]} $ratio_trade ${month_mds_modify_fo[$sym]}" >> $tmp_file1
#      fi                                                                                
done
echo "ratio computation done"

#[[ $product_count != `wc -l $tmp_file1 | cut -d' ' -f1` ]] && ErrorMail 
sort -n -rk 4 $tmp_file1 > $tmp_file2
echo "sorting done"

for prod in `awk '{print $1}' $tmp_file2`; do
  grep $prod $datasource >> $tmp_file3
done

awk 'NR==FNR {sym[$1]=$2; next} {print sym[$1],$2,$3,$4,$5,$6,$7,$8,$9,$10,$11}' $tmp_file3 $tmp_file2 > $tmp_file4

>$mail_file
echo -e "<p>$today_ Product Count: $product_count</p>" >> $mail_file
echo -e "<br/><table border="1"><thead><th> Products </th><th> Total </th><th> Exec </th><th> Ratio </th><th> Modify </th><th> ModifyOF </th><th> ModifyOf% </th><th> Total_MDS_MSG </th><th> Total_Trade </th><th> Trade_Ratio </th><th> Total_MDS_Modify </th><tbody>" >> $mail_file
while IFS=' ' read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9 f10
do
    echo "<tr><td>$sym</td><td>$f1</td><td>$f2</td><td>$f3</td><td>$f4</td><td>$f5</td><td>$f6</td><td>$f7</td><td>$f8</td><td>$f9</td><td>$f10</td></tr>" >> $mail_file
done < $tmp_file4

echo -e "</tbody></table>" >> $mail_file
echo -e "<br/><br/>" >> $mail_file


SendMail
echo "mail sent"
echo "generating html ors report for fo"

/home/dvcinfra/important/Generate_ors_detail/OrsReportFO/generate_ors_report_fo.sh $today_

echo "generated html ors report for fo"

