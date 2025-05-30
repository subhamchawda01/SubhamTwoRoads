#!/bin/bash
# destpath ,work_dir, path, 
declare -A day_modify_fo
declare -A day_modifyof_fo
declare -A day_product_fo
declare -A day_exec_fo
declare -A day_mds_msg_fo
declare -A day_trade_fo
declare -A day_mds_modify_fo
declare -A day_mds_modify_price_fo
declare -A month_modify_fo
declare -A month_modifyof_fo
declare -A month_product_fo
declare -A month_exec_fo
declare -A month_mds_msg_fo
declare -A month_trade_fo
declare -A month_mds_modify_fo
declare -A month_mds_modify_price_fo

print_msg_and_exit () {
  echo $* ;
  exit ;
}

PerDayORSReportsCash () { 
  PerDayORS_cash=$destpath"daily_ors_file/"$yyyymmdd".fo.index.html"
  > $PerDayORS_cash ;
  chmod 777 $PerDayORS_cash
  cat $path"daily_ors_report_header.txt" | sed 's/ORS REPORTS/ORS REPORTS [ '$yyyymmdd' ] /g' >> $PerDayORS_cash ;
  echo "<th>Symbol</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th></tr></thead><tbody>" >> $PerDayORS_cash

  for sym in "${!day_product_fo[@]}"; do
    day_ratio_order=${day_product_fo[$sym]};
    day_percent_modify=0
    day_ratio_trade=${day_mds_msg_fo[$sym]};
    [[ ${day_exec_fo[$sym]} == 0 ]] || day_ratio_order=$((${day_product_fo[$sym]} / ${day_exec_fo[$sym]}))
    [[ ${day_modifyof_fo[$sym]} == 0 ]] || day_percent_modify=$((${day_modifyof_fo[$sym]} * 100 / ${day_modify_fo[$sym]}))
    [[ ${day_trade_fo[$sym]} == 0 ]] || day_ratio_trade=$((${day_mds_msg_fo[$sym]} / ${day_trade_fo[$sym]}))
    symbol=`grep $sym $datasource | awk '{print $NF}'`
    modify_of_to_modify_price=$(( ${day_modifyof_fo[$sym]} * 100 / ${day_mds_modify_price_fo[$sym]} ))
    echo "<tr><td><b>$symbol</b></td><td>${day_product_fo[$sym]}</td><td>${day_exec_fo[$sym]}</td><td>$day_ratio_order</td><td>${day_modify_fo[$sym]}</td><td>${day_modifyof_fo[$sym]}</td><td>$day_percent_modify</td><td>${day_mds_msg_fo[$sym]}</td><td>${day_trade_fo[$sym]}</td><td>$day_ratio_trade</td><td>${day_mds_modify_fo[$sym]}</td><td>${day_mds_modify_price_fo[$sym]}</td><td>$modify_of_to_modify_price</td></tr>" >> $PerDayORS_cash
  done
  cat $path"daily_ors_report_footer.txt" >> $PerDayORS_cash
}

month_file_gen(){
  month_cash=$destpath"daily_ors_file/"$yyyymm".fo.index.html"
  >$month_cash;
  chmod 777 $month_cash;
  cat $path"daily_ors_report_header.txt" | sed "s/ORS REPORTS/ORS REPORTS MONTHLY ${yyyymm} FO /g" >> $month_cash ;
  echo "<th>Symbol</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th></tr></thead><tbody>" >> $month_cash ;

  for sym in "${!month_product_fo[@]}"; do
    ratio_order=${month_product_fo[$sym]};
    percent_modify=0
    ratio_trade=${month_mds_msg_fo[$sym]};
    [[ ${month_exec_fo[$sym]} == 0 ]] || ratio_order=$(( ${month_product_fo[$sym]} / ${month_exec_fo[$sym]} ))
    [[ ${month_modifyof_fo[$sym]} == 0 ]] || percent_modify=$(( ${month_modifyof_fo[$sym]} * 100 / ${month_modify_fo[$sym]} ))
    [[ ${month_trade_fo[$sym]} == 0 ]] || ratio_trade=$(( ${month_mds_msg_fo[$sym]} / ${month_trade_fo[$sym]} ))
    symbol=`grep $sym $datasource | awk '{print $NF}'`
    modify_of_to_modify_price=$(( ${month_modifyof_fo[$sym]} * 100 / ${month_mds_modify_price_fo[$sym]} ))
    echo "<tr><td><b>$symbol</b></td><td>${month_product_fo[$sym]}</td><td>${month_exec_fo[$sym]}</td><td>$ratio_order</td><td>${month_modify_fo[$sym]}</td><td>${month_modifyof_fo[$sym]}</td><td>$percent_modify</td><td>${month_mds_msg_fo[$sym]}</td><td>${month_trade_fo[$sym]}</td><td>$ratio_trade</td><td>${month_mds_modify_fo[$sym]}</td><td>${month_mds_modify_price_fo[$sym]}</td><td>$modify_of_to_modify_price</td></tr>" >> $month_cash
  done
  cat $path"daily_ors_report_footer.txt" >> $month_cash
}

init () {

#path="/home/dvcinfra/important/subham/";
  path="/home/dvcinfra/important/Generate_ors_detail/OrsReportFO/";
  destpath="/var/www/html/ors_report_fo/"
  work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details_FO/'
  datasource="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
#dailyreport="/home/dvcinfra/important/subham/dailyreport.txt"
  dailyreport="${path}dailyreport.txt"
  REPORTING_FILE=$destpath"index.html" ;
  chmod 777 $REPORTING_FILE
  >$REPORTING_FILE ;

  mkdir -p $destpath ;
  mkdir -p ${destpath}daily_ors_file;
  cat $path"generate_ors_header.txt" > $REPORTING_FILE ;
  echo "<h3>ORS REPORTS OF LAST 30 DAYS</h3></div>" >> $REPORTING_FILE
  echo "<table class="table table-striped" style='border: 1px solid grey;margin-bottom: 0px;'><thead><tr>" >> $REPORTING_FILE
  echo "<th>Month</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th></tr></thead><tbody>" >> $REPORTING_FILE
  yyyymm=${today:0:6}
  yyyymmdd=$today
  echo "Month Computing $yyyymm"
  for foFile in `ls -t $work_dir/* | grep $yyyymm | head -30 | grep "_*fo"`;
  do
    current_day_flag=`echo $foFile | grep $yyyymmdd | wc -l`
    echo "file: $foFile"
    while IFS=' ' read -r sym_t f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 f20 f21 f22 f23 f24 f25 f26
    do
      if [ $current_day_flag != "0" ]; then
        if [ ${day_product_fo[$sym_t]} ]; then
          day_modify_fo[$sym_t]=$(( ${day_modify_fo[$sym_t]} + $f17))
          day_total_modify=$(( $f17 + $day_total_modify))
          day_modifyof_fo[$sym_t]=$(( ${day_modifyof_fo[$sym_t]} + $f18))
          day_total_modifyof=$(( $f18 + $day_total_modifyof))
          day_product_fo[$sym_t]=$(( ${day_product_fo[$sym_t]} + $f19))
          day_total_order=$(( $f19 + $day_total_order))
          day_exec_fo[$sym_t]=$(( ${day_exec_fo[$sym_t]} +  $f20))
          day_total_exec=$(( $f20 + $day_total_exec))
          day_mds_msg_fo[$sym_t]=$(( ${day_mds_msg_fo[$sym_t]} +  $f22))
          day_total_mds_msg=$(( $f22 + $day_total_mds_msg))
          day_trade_fo[$sym_t]=$(( ${day_trade_fo[$sym_t]} +  $f23))
          day_total_trade=$(( $f23 + $day_total_trade))
          day_mds_modify_fo[$sym_t]=$(( ${day_mds_modify_fo[$sym_t]} +  $f25))
          day_total_mds_modify=$(( $f25 + $day_total_mds_modify))
          day_mds_modify_price_fo[$sym_t]=$(( ${day_mds_modify_price_fo[$sym_t]} +  $f26))
          day_total_mds_modify_price=$(( $f26 + $day_total_mds_modify_price))
        else
          day_modify_fo[$sym_t]=$f17
          day_total_modify=$(( f17 + day_total_modify))
          day_modifyof_fo[$sym_t]=$f18
          day_total_modifyof=$(( f18 + day_total_modifyof))
          day_product_fo[$sym_t]=$f19
          day_total_order=$(( f19 + day_total_order))
          day_exec_fo[$sym_t]=$f20
          day_total_exec=$(( f20 + day_total_exec))
          day_mds_msg_fo[$sym_t]=$f22
          day_total_mds_msg=$(( f22 + day_total_mds_msg))
          day_trade_fo[$sym_t]=$f23
          day_total_trade=$(( f23 + day_total_trade))
          day_mds_modify_fo[$sym_t]=$f25
          day_total_mds_modify=$(( f25 + day_total_mds_modify))
          day_mds_modify_price_fo[$sym_t]=$f26
          day_total_mds_modify_price=$(( f26 + day_total_mds_modify_price))
        fi
      fi
      if [ ${month_product_fo[$sym_t]} ]; then
        month_modify_fo[$sym_t]=$(( ${month_modify_fo[$sym_t]} + $f17))
        total_modify=$(( $f17 + $total_modify))
        month_modifyof_fo[$sym_t]=$(( ${month_modifyof_fo[$sym_t]} + $f18))
        total_modifyof=$(( $f18 + $total_modifyof))
        month_product_fo[$sym_t]=$(( ${month_product_fo[$sym_t]} + $f19))
        total_order=$(( $f19 + $total_order))
        month_exec_fo[$sym_t]=$(( ${month_exec_fo[$sym_t]} + $f20))
        total_exec=$(( $f20 + $total_exec))
        month_mds_msg_fo[$sym_t]=$(( ${month_mds_msg_fo[$sym_t]} + $f22))
        total_mds_msg=$(( $f22 + $total_mds_msg))
        month_trade_fo[$sym_t]=$(( ${month_trade_fo[$sym_t]} + $f23))
        total_trade=$(( $f23 + $total_trade))
        month_mds_modify_fo[$sym_t]=$(( ${month_mds_modify_fo[$sym_t]} + $f25))
        total_mds_modify=$(( $f25 + $total_mds_modify))
        month_mds_modify_price_fo[$sym_t]=$(( ${month_mds_modify_price_fo[$sym_t]} + $f26))
        total_mds_modify_price=$(( $f26 + $total_mds_modify_price))
      else
        month_modify_fo[$sym_t]=$f17
        total_modify=$(( f17 + total_modify))
        month_modifyof_fo[$sym_t]=$f18
        total_modifyof=$(( f18 + total_modifyof))
        month_product_fo[$sym_t]=$f19
        total_order=$(( f19 + total_order))
        month_exec_fo[$sym_t]=$f20
        total_exec=$(( f20 + total_exec))
        month_mds_msg_fo[$sym_t]=$f22
        total_mds_msg=$(( f22 + total_mds_msg))
        month_trade_fo[$sym_t]=$f23
        total_trade=$(( f23 + total_trade))
        month_mds_modify_fo[$sym_t]=$f25
        total_mds_modify=$(( f25 + total_mds_modify))
        month_mds_modify_price_fo[$sym_t]=$f26
        total_mds_modify_price=$(( f26 + total_mds_modify_price))
      fi
    done < $foFile
  done

  month_file_gen;
  echo $yyyymm
  total_ratio_order=$total_order;
  total_percent_modify=0
  total_ratio_trade=$total_mds_msg;
  [[ $total_exec == 0 ]] || total_ratio_order=$(( total_order / total_exec ))
  [[ $total_modify == 0 ]] || total_percent_modify=$(( total_modifyof * 100 / total_modify ))
  [[ $total_trade == 0 ]] || total_ratio_trade=$(( total_mds_msg / total_trade ))
  modify_of_to_modify_price=$(( ${total_modifyof} * 100 / ${total_mds_modify_price} ))
  echo "<tr><td><b><a href="daily_ors_file/"$yyyymm".fo.index.html" style="color:blue">$yyyymm</a></b></td><td>$total_order</td><td>$total_exec</td><td>$total_ratio_order</td><td>$total_modify</td><td>$total_modifyof</td><td>$total_percent_modify</td><td>$total_mds_msg</td><td>$total_trade</td><td>$total_ratio_trade</td><td>$total_mds_modify</td><td>$total_mds_modify_price</td><td>$modify_of_to_modify_price</td>" >> $REPORTING_FILE
  echo "</tbody></table>" >> $REPORTING_FILE
  echo "<div class='row header' style='text-align:center;color:green'><h3>ORS REPORTS PER DAY</h3></div>" >> $REPORTING_FILE

  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Date</th><th>Total_Order</th><th>Total_Exec</th><th>Exec_Ratio</th><th>Total_Modify</th><th>Total_Modify_OF</th><th>ModifyOF%</th><th>Total_MDS_Msg</th><th>Total_Trade</th><th>Trade_Ratio</th><th>Total_Trade_Modify</th><th>Total_Trade_Modify_Price</th><th>Total_MDS_Modify_Price_OF%</th>" >> $REPORTING_FILE;
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;

  PerDayORSReportsCash ;

  day_total_ratio_order=$day_total_order;
  day_total_percent_modify=0
  day_total_ratio_trade=$day_total_mds_msg;
  echo "$day_total_exec $day_total_order"
  [[ $day_total_exec == 0 ]] || day_total_ratio_order=$(( day_total_order / day_total_exec ))
  [[ $day_total_modify == 0 ]] || day_total_percent_modify=$(( day_total_modifyof * 100 / day_total_modify ))
  [[ $day_total_trade == 0 ]] || day_total_ratio_trade=$(( day_total_mds_msg / day_total_trade ))
  modify_of_to_modify_price=$(( ${day_total_modifyof} * 100 / ${day_total_mds_modify_price} ))
  echo "<tr><td><a href ="daily_ors_file/"$yyyymmdd".fo.index.html" style="color:blue">$yyyymmdd</a></td><td>$day_total_order</td><td>$day_total_exec</td><td>$day_total_ratio_order</td><td>$day_total_modify</td><td>$day_total_modifyof</td><td>$day_total_percent_modify</td><td>$day_total_mds_msg</td><td>$day_total_trade</td><td>$day_total_ratio_trade</td><td>$day_total_mds_modify</td><td>$day_total_mds_modify_price</td><td>$modify_of_to_modify_price</td>" >> $dailyreport;

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
