#!/bin/bash
# destpath ,work_dir, path, 
declare -A day_modify_cm
declare -A day_modifyof_cm
declare -A day_product_cm
declare -A day_exec_cm
declare -A day_product_price_cm
declare -A day_exec_price_cm
declare -A day_mds_msg_cm
declare -A day_trade_cm
declare -A day_mds_modify_cm
declare -A day_mds_modify_price_cm
declare -A month_modify_cm
declare -A month_modifyof_cm
declare -A month_product_cm
declare -A month_exec_cm
declare -A month_product_price_cm
declare -A month_exec_price_cm
declare -A month_mds_msg_cm
declare -A month_trade_cm
declare -A month_mds_modify_cm
declare -A month_mds_modify_price_cm

print_msg_and_exit () {
  echo $* ;
  exit ;
}

PerDayORSReportsCash () { 
  PerDayORS_cash=$destpath"daily_ors_file/"$yyyymmdd".cm.index.html"
  > $PerDayORS_cash ;
  chmod 777 $PerDayORS_cash
  cat $path"daily_ors_report_header.txt" | sed 's/ORS REPORTS/ORS REPORTS [ '$yyyymmdd' ] /g' >> $PerDayORS_cash ;
  echo "<th>Symbol</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Order_Price</th><th>Total_Exec_Price</th><th>Exec_Ratio_Price</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th></tr></thead><tbody>" >> $PerDayORS_cash

  for sym in "${!day_product_cm[@]}"; do
    day_ratio_order=${day_product_cm[$sym]};
    day_ratio_order_price=${day_product_price_cm[$sym]};
    day_percent_modify=0
    day_ratio_trade=${day_mds_msg_cm[$sym]};
    [[ ${day_exec_cm[$sym]} == 0 ]] || day_ratio_order=$((${day_product_cm[$sym]} / ${day_exec_cm[$sym]}))
    [[ ${day_exec_price_cm[$sym]} == "0" ]] || [[ ${day_exec_price_cm[$sym]} == "0.00" ]] || day_ratio_order_price=$(echo "${day_product_price_cm[$sym]} / ${day_exec_price_cm[$sym]}" | bc)
    [[ ${day_modifyof_cm[$sym]} == 0 ]] || day_percent_modify=$((${day_modifyof_cm[$sym]} * 100 / ${day_modify_cm[$sym]}))
    [[ ${day_trade_cm[$sym]} == 0 ]] || day_ratio_trade=$((${day_mds_msg_cm[$sym]} / ${day_trade_cm[$sym]}))
    symbol=$sym
    modify_of_to_modify_price=$(( ${day_modifyof_cm[$sym]} * 100 / ${day_mds_modify_price_cm[$sym]} ))
    echo "<tr><td><b>$symbol</b></td><td>${day_product_cm[$sym]}</td><td>${day_exec_cm[$sym]}</td><td>$day_ratio_order</td><td>${day_product_price_cm[$sym]}</td><td>${day_exec_price_cm[$sym]}</td><td>$day_ratio_order_price</td><td>${day_modify_cm[$sym]}</td><td>${day_modifyof_cm[$sym]}</td><td>$day_percent_modify</td><td>${day_mds_msg_cm[$sym]}</td><td>${day_trade_cm[$sym]}</td><td>$day_ratio_trade</td><td>${day_mds_modify_cm[$sym]}</td><td>${day_mds_modify_price_cm[$sym]}</td><td>$modify_of_to_modify_price</td></tr>" >> $PerDayORS_cash
  done
  cat $path"daily_ors_report_footer.txt" >> $PerDayORS_cash
}

month_file_gen(){
  month_cash=$destpath"daily_ors_file/"$yyyymm".cm.index.html"
  >$month_cash;
  chmod 777 $month_cash;
  cat $path"daily_ors_report_header.txt" | sed "s/ORS REPORTS/ORS REPORTS MONTHLY ${yyyymm} CM /g" >> $month_cash ;
  echo "<th>Symbol</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Order</th><th>Total_Exec_Price</th><th>Exec_Ratio_Price</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th></tr></thead><tbody>" >> $month_cash ;

  for sym in "${!month_product_cm[@]}"; do
    ratio_order=${month_product_cm[$sym]};
    ratio_order_price=${month_product_price_cm[$sym]};
    percent_modify=0
    ratio_trade=${month_mds_msg_cm[$sym]};
    [[ ${month_exec_cm[$sym]} == 0 ]] || ratio_order=$(( ${month_product_cm[$sym]} / ${month_exec_cm[$sym]} ))
    [[ ${month_exec_price_cm[$sym]} == "0" ]] || [[ ${month_exec_price_cm[$sym]} == "0.00" ]] || ratio_order_price=$(echo "${month_product_price_cm[$sym]} / ${month_exec_price_cm[$sym]}" | bc)
    [[ ${month_modifyof_cm[$sym]} == 0 ]] || percent_modify=$(( ${month_modifyof_cm[$sym]} * 100 / ${month_modify_cm[$sym]} ))
    [[ ${month_trade_cm[$sym]} == 0 ]] || ratio_trade=$(( ${month_mds_msg_cm[$sym]} / ${month_trade_cm[$sym]} ))
    symbol=$sym
    modify_of_to_modify_price=$(( ${month_modifyof_cm[$sym]} * 100 / ${month_mds_modify_price_cm[$sym]} ))
    echo "<tr><td><b>$symbol</b></td><td>${month_product_cm[$sym]}</td><td>${month_exec_cm[$sym]}</td><td>$ratio_order</td><td>${month_product_price_cm[$sym]}</td><td>${month_exec_price_cm[$sym]}</td><td>$ratio_order_price</td><td>${month_modify_cm[$sym]}</td><td>${month_modifyof_cm[$sym]}</td><td>$percent_modify</td><td>${month_mds_msg_cm[$sym]}</td><td>${month_trade_cm[$sym]}</td><td>$ratio_trade</td><td>${month_mds_modify_cm[$sym]}</td><td>${month_mds_modify_price_cm[$sym]}</td><td>$modify_of_to_modify_price</td></tr>" >> $month_cash
  done
  cat $path"daily_ors_report_footer.txt" >> $month_cash
}

init () {

#path="/home/dvcinfra/important/subham/";
  path="/home/dvcinfra/important/Generate_ors_detail/OrsReportCM/";
  destpath="/var/www/html/ors_report_cm/"
  work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details_CM/'
  dailyreport="${path}dailyreport.txt"
  REPORTING_FILE=$destpath"index.html" ;
  chmod 777 $REPORTING_FILE
  >$REPORTING_FILE ;

  mkdir -p $destpath ;
  mkdir -p ${destpath}daily_ors_file;
  cat $path"generate_ors_header.txt" > $REPORTING_FILE ;
  echo "<h3>ORS REPORTS OF LAST 30 DAYS</h3></div>" >> $REPORTING_FILE
  echo "<table class="table table-striped" style='border: 1px solid grey;margin-bottom: 0px;'><thead><tr>" >> $REPORTING_FILE
  echo "<th>Month</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Order_Price</th><th>Total_Exec_Price</th><th>Exec_Ratio_Price</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th></tr></thead><tbody>" >> $REPORTING_FILE
  yyyymm=${today:0:6}
  yyyymmdd=$today
  echo "Month Computing $yyyymm"
  day_total_order_price=0; day_total_exec_price=0; 
  total_order_price=0; total_exec_price=0; 

  for cmFile in `ls -t $work_dir/* | grep $yyyymm | head -30 | grep "_*cm"`;
  do
    current_day_flag=`echo $cmFile | grep $yyyymmdd | wc -l`
    echo "file: $cmFile"
    while IFS=' ' read -r sym_t f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 f20 f21 f22 f23 f24 f25 f26 f27 f28 f29 f30 f31 f32
    do
      if [ $current_day_flag != "0" ]; then
        if [ ${day_product_cm[$sym_t]} ]; then
          day_modify_cm[$sym_t]=$(( ${day_modify_cm[$sym_t]} + $f21))
          day_total_modify=$(( $f21 + $day_total_modify))
          day_modifyof_cm[$sym_t]=$(( ${day_modifyof_cm[$sym_t]} + $f22))
          day_total_modifyof=$(( $f22 + $day_total_modifyof))
          day_product_cm[$sym_t]=$(( ${day_product_cm[$sym_t]} + $f23))
          day_total_order=$(( $f23 + $day_total_order))
          day_exec_cm[$sym_t]=$(( ${day_exec_cm[$sym_t]} +  $f24))
          day_total_exec=$(( $f24 + $day_total_exec))
          day_product_price_cm[$sym_t]=$(( ${day_product_price_cm[$sym_t]} + $f25))
          day_total_order_price=$(( $f25 + $day_total_order_price))
          day_exec_price_cm[$sym_t]=$(( ${day_exec_price_cm[$sym_t]} +  $f26))
          day_total_exec_price=$(( $f26 + $day_total_exec_price))
          day_mds_msg_cm[$sym_t]=$(( ${day_mds_msg_cm[$sym_t]} +  $f28))
          day_total_mds_msg=$(( $f28 + $day_total_mds_msg))
          day_trade_cm[$sym_t]=$(( ${day_trade_cm[$sym_t]} +  $f29))
          day_total_trade=$(( $f29 + $day_total_trade))
          day_mds_modify_cm[$sym_t]=$(( ${day_mds_modify_cm[$sym_t]} +  $f31))
          day_total_mds_modify=$(( $f31 + $day_total_mds_modify))
          day_mds_modify_price_cm[$sym_t]=$(( ${day_mds_modify_price_cm[$sym_t]} +  $f32))
          day_total_mds_modify_price=$(( $f32 + $day_total_mds_modify_price))
        else
          day_modify_cm[$sym_t]=$f21
          day_total_modify=$(( f21 + day_total_modify))
          day_modifyof_cm[$sym_t]=$f22
          day_total_modifyof=$(( f22 + day_total_modifyof))
          day_product_cm[$sym_t]=$f23
          day_total_order=$(( f23 + day_total_order))
          day_exec_cm[$sym_t]=$f24
          day_total_exec=$(( f24 + day_total_exec))
          day_product_price_cm[$sym_t]=$f25
          day_total_order_price=$(echo "$f25 + $day_total_order_price" | bc)
          day_exec_price_cm[$sym_t]=$f26
          day_total_exec_price=$(echo "$f26 + $day_total_exec_price" | bc)
          day_mds_msg_cm[$sym_t]=$f28
          day_total_mds_msg=$(( f28 + day_total_mds_msg))
          day_trade_cm[$sym_t]=$f29
          day_total_trade=$(( f29 + day_total_trade))
          day_mds_modify_cm[$sym_t]=$f31
          day_total_mds_modify=$(( f31 + day_total_mds_modify))
          day_mds_modify_price_cm[$sym_t]=$f32
          day_total_mds_modify_price=$(( f32 + day_total_mds_modify_price))
        fi
      fi
      if [ ${month_product_cm[$sym_t]} ]; then
        month_modify_cm[$sym_t]=$(( ${month_modify_cm[$sym_t]} + $f21))
        total_modify=$(( $f21 + $total_modify))
        month_modifyof_cm[$sym_t]=$(( ${month_modifyof_cm[$sym_t]} + $f22))
        total_modifyof=$(( $f22 + $total_modifyof))
        month_product_cm[$sym_t]=$(( ${month_product_cm[$sym_t]} + $f23))
        total_order=$(( $f23 + $total_order))
        month_exec_cm[$sym_t]=$(( ${month_exec_cm[$sym_t]} + $f24))
        total_exec=$(( $f24 + $total_exec))
        month_product_price_cm[$sym_t]=$(echo "${month_product_price_cm[$sym_t]} + $f25" | bc)
        total_order_price=$(echo "$f25 + $total_order_price" | bc)
        month_exec_price_cm[$sym_t]=$(echo "${month_exec_price_cm[$sym_t]} + $f26" | bc)
        total_exec_price=$(echo "$f26 + $total_exec_price" | bc)
        month_mds_msg_cm[$sym_t]=$(( ${month_mds_msg_cm[$sym_t]} + $f28))
        total_mds_msg=$(( $f28 + $total_mds_msg))
        month_trade_cm[$sym_t]=$(( ${month_trade_cm[$sym_t]} + $f29))
        total_trade=$(( $f29 + $total_trade))
        month_mds_modify_cm[$sym_t]=$(( ${month_mds_modify_cm[$sym_t]} + $f31))
        total_mds_modify=$(( $f31 + $total_mds_modify))
        month_mds_modify_price_cm[$sym_t]=$(( ${month_mds_modify_price_cm[$sym_t]} + $f32))
        total_mds_modify_price=$(( $f32 + $total_mds_modify_price))
      else
        month_modify_cm[$sym_t]=$f21
        total_modify=$(( f21 + total_modify))
        month_modifyof_cm[$sym_t]=$f22
        total_modifyof=$(( f22 + total_modifyof))
        month_product_cm[$sym_t]=$f23
        total_order=$(( f23 + total_order))
        month_exec_cm[$sym_t]=$f24
        total_exec=$(( f24 + total_exec))
        month_product_price_cm[$sym_t]=$f25
        total_order_price=$(echo "$f25 + $total_order_price" | bc)
        month_exec_price_cm[$sym_t]=$f26
        total_exec_price=$(echo "$f26 + $total_exec_price" | bc)
        month_mds_msg_cm[$sym_t]=$f28
        total_mds_msg=$(( f28 + total_mds_msg))
        month_trade_cm[$sym_t]=$f29
        total_trade=$(( f29 + total_trade))
        month_mds_modify_cm[$sym_t]=$f31
        total_mds_modify=$(( f31 + total_mds_modify))
        month_mds_modify_price_cm[$sym_t]=$f32
        total_mds_modify_price=$(( f32 + total_mds_modify_price))
      fi
    done < $cmFile
  done

  month_file_gen;
  echo $yyyymm
  total_ratio_order=$total_order;
  total_ratio_order_price=$total_order_price;
  total_percent_modify=0
  total_ratio_trade=$total_mds_msg;
  [[ $total_exec == 0 ]] || total_ratio_order=$(( total_order / total_exec ))
  [[ $total_exec_price == "0" ]] || [[ $total_exec_price == "0.00" ]] || total_ratio_order_price=$(echo "$total_order_price / $total_exec_price" | bc)
  [[ $total_modify == 0 ]] || total_percent_modify=$(( total_modifyof * 100 / total_modify ))
  [[ $total_trade == 0 ]] || total_ratio_trade=$(( total_mds_msg / total_trade ))
  modify_of_to_modify_price=$(( ${total_modifyof} * 100 / ${total_mds_modify_price} ))
  echo "<tr><td><b><a href="daily_ors_file/"$yyyymm".cm.index.html" style="color:blue">$yyyymm</a></b></td><td>$total_order</td><td>$total_exec</td><td>$total_ratio_order</td><td>$total_order_price</td><td>$total_exec_price</td><td>$total_ratio_order_price</td><td>$total_modify</td><td>$total_modifyof</td><td>$total_percent_modify</td><td>$total_mds_msg</td><td>$total_trade</td><td>$total_ratio_trade</td><td>$total_mds_modify</td><td>$total_mds_modify_price</td><td>$modify_of_to_modify_price</td>" >> $REPORTING_FILE
  echo "</tbody></table>" >> $REPORTING_FILE
  echo "<div class='row header' style='text-align:center;color:green'><h3>ORS REPORTS PER DAY</h3></div>" >> $REPORTING_FILE

  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Date</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Order_Price</th><th>Total_Exec_Price</th><th>Exec_Ratio_Price</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th>" >> $REPORTING_FILE;
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;

  PerDayORSReportsCash ;

  day_total_ratio_order=$day_total_order;
  day_total_ratio_order_price=$day_total_order_price;
  day_total_percent_modify=0
  day_total_ratio_trade=$day_total_mds_msg;
  [[ $day_total_exec == 0 ]] || day_total_ratio_order=$(( day_total_order / day_total_exec ))
  [[ $day_total_exec_price == "0" ]] || [[ $day_total_exec_price == "0.00" ]] || day_total_ratio_order_price=$(echo "$day_total_order_price / $day_total_exec_price" | bc)
  [[ $day_total_modify == 0 ]] || day_total_percent_modify=$(( day_total_modifyof * 100 / day_total_modify ))
  [[ $day_total_trade == 0 ]] || day_total_ratio_trade=$(( day_total_mds_msg / day_total_trade ))
  modify_of_to_modify_price=$(( ${day_total_modifyof} * 100 / ${day_total_mds_modify_price} ))
  echo "<tr><td><a href ="daily_ors_file/"$yyyymmdd".cm.index.html" style="color:blue">$yyyymmdd</a></td><td>$day_total_order</td><td>$day_total_exec</td><td>$day_total_ratio_order</td><td>$day_total_order_price</td><td>$day_total_exec_price</td><td>$day_total_ratio_order_price</td><td>$day_total_modify</td><td>$day_total_modifyof</td><td>$day_total_percent_modify</td><td>$day_total_mds_msg</td><td>$day_total_trade</td><td>$day_total_ratio_trade</td><td>$day_total_mds_modify</td><td>$day_total_mds_modify_price</td><td>$modify_of_to_modify_price</td>" >> $dailyreport;

  cat $dailyreport >> $REPORTING_FILE;
  cat $path"generate_ors_footer.txt" >> $REPORTING_FILE;
}

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 Date todays" ;
  exit ;
fi

today=$1;
init $*
