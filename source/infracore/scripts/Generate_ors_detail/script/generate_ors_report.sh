#!/bin/bash
declare -A month_product_cash_tot
declare -A month_exec_cash_tot
declare -A month_product_cash_ind16
declare -A month_exec_cash_ind16
declare -A month_product_cash_ind17
declare -A month_exec_cash_ind17
declare -A month_product_cash_ind18
declare -A month_exec_cash_ind18

print_msg_and_exit () {
  echo $* ;
  exit ;
}

PerDayORSReportsCash () { 
  PerDayORS_cash=$destpath"daily_ors_file/"$day".cash.index.html"
  > $PerDayORS_cash ;
  chmod +777 $PerDayORS_cash
  cat $path"daily_ors_report_header.txt" | sed 's/ORS REPORTS/ORS REPORTS [ '$day' ] /g' >> $PerDayORS_cash ;
  echo "<th>Symbol</th><th>IND16 Order</th><th>IND16 Exec</th><th>IND16 Ratio</th><th>IND17 Order</th><th>IND17 Exec</th><th>IND17 Ratio</th><th>IND18 Order</th><th>IND18 Exec</th><th>IND18 Ratio</th><th>TotalOrder</th><th>TotalExec</th><th>TotalRatio</th></tr></thead><tbody>" >> $PerDayORS_cash
  while read sym f1 f2 f3 f4 f5 f6 f7 f8 f9
  do
         if [[ $f8 == '' ]]; then
           continue;
         fi
	 ratio_ind16=-1
         [[ $f2 == 0 ]] || ratio_ind16=$(( $f1 / $f2 ))
         sum_cash_order_ind16=$(( $sum_cash_order_ind16 + $f1 ))
         sum_cash_exec_ind16=$(( $sum_cash_exec_ind16 + $f2 ))
         ratio_ind17=-1
         [[ $f4 == 0 ]] || ratio_ind17=$(( $f3 / $f4 ))
         sum_cash_order_ind17=$(( $sum_cash_order_ind17 + $f3 ))
         sum_cash_exec_ind17=$(( $sum_cash_exec_ind17 + $f4 ))
         ratio_ind18=-1
         [[ $f6 == 0 ]] || ratio_ind18=$(( $f5 / $f6 ))
         sum_cash_order_ind18=$(( $sum_cash_order_ind18 + $f5 ))
         sum_cash_exec_ind18=$(( $sum_cash_exec_ind18 + $f6 ))
         ratio_tot=-1
         [[ $f8 == 0 ]] || ratio_tot=$(( $f7 / $f8 ))
         sum_cash_order_tot=$(( $sum_cash_order_tot + $f7 ))
         sum_cash_exec_tot=$(( $sum_cash_exec_tot + $f8 ))

         echo "<tr><td><b>$sym</b></td><td>$f1</td><td>$f2</td><td>$ratio_ind16</td><td>$f3</td><td>$f4</td><td>$ratio_ind17</td><td>$f5</td><td>$f6</td><td>$ratio_ind18</td><td>$f7</td><td>$f8</td><td>$ratio_tot</td></tr>" >> $PerDayORS_cash
  done < $cashFile
  cat $path"daily_ors_report_footer.txt" >> $PerDayORS_cash
}

month_file_gen(){
  month_cash=$destpath"daily_ors_file/"$yyyymm".cash.index.html"
  cash_monthly_file="/home/dvcinfra/important/Generate_ors_detail/Product_Details_Monthly/"$yyyymm".cash"
  >$cash_monthly_file
  >$month_cash;
  chmod +777 $month_cash;
  cat $path"daily_ors_report_header.txt" | sed "s/ORS REPORTS/ORS REPORTS MONTHLY ${yyyymm} CASH /g" >> $month_cash ;
  echo "<th>Symbol</th><th>IND16 Order</th><th>IND16 Exec</th><th>IND16 Ratio</th><th>IND17 Order</th><th>IND17 Exec</th><th>IND17 Ratio</th><th>IND18 Order</th><th>IND18 Exec</th><th>IND18 Ratio</th><th>TotalOrder</th><th>TotalExec</th><th>TotalRatio</th></tr></thead><tbody>" >> $month_cash ;
  sum_cash_order_tot=0;sum_cash_exec_tot=1; sum_cash_order_ind16=0;sum_cash_exec_ind16=1; sum_cash_order_ind17=0;sum_cash_exec_ind17=1; sum_cash_order_ind18=0;sum_cash_exec_ind18=1;

  for sym in "${!month_product_cash_tot[@]}"; do
        ratio_ind16=-1
        sum_cash_order_ind16=$(( $sum_cash_order_ind16 + ${month_product_cash_ind16[$sym]} ))
        sum_cash_exec_ind16=$(( $sum_cash_exec_ind16 + ${month_exec_cash_ind16[$sym]} ))
        [[ ${month_exec_cash_ind16[$sym]} == 0 ]] || ratio_ind16=$((${month_product_cash_ind16[$sym]} / ${month_exec_cash_ind16[$sym]}))
        ratio_ind17=-1
        sum_cash_order_ind17=$(( $sum_cash_order_ind17 + ${month_product_cash_ind17[$sym]} ))
        sum_cash_exec_ind17=$(( $sum_cash_exec_ind17 + ${month_exec_cash_ind17[$sym]} ))
        [[ ${month_exec_cash_ind17[$sym]} == 0 ]] || ratio_ind17=$((${month_product_cash_ind17[$sym]} / ${month_exec_cash_ind17[$sym]}))
        ratio_ind18=-1
        sum_cash_order_ind18=$(( $sum_cash_order_ind18 + ${month_product_cash_ind18[$sym]} ))
        sum_cash_exec_ind18=$(( $sum_cash_exec_ind18 + ${month_exec_cash_ind18[$sym]} ))
        [[ ${month_exec_cash_ind18[$sym]} == 0 ]] || ratio_ind18=$((${month_product_cash_ind18[$sym]} / ${month_exec_cash_ind18[$sym]}))
        ratio_tot=-1
        sum_cash_order_tot=$(( $sum_cash_order_tot + ${month_product_cash_tot[$sym]} ))
        sum_cash_exec_tot=$(( $sum_cash_exec_tot + ${month_exec_cash_tot[$sym]} ))
        [[ ${month_exec_cash_tot[$sym]} == 0 ]] || ratio_tot=$((${month_product_cash_tot[$sym]} / ${month_exec_cash_tot[$sym]}))

        echo "$sym ${month_product_cash_ind16[$sym]} ${month_exec_cash_ind16[$sym]} ${month_product_cash_ind17[$sym]} ${month_exec_cash_ind17[$sym]} ${month_product_cash_ind18[$sym]} ${month_exec_cash_ind18[$sym]} ${month_product_cash_tot[$sym]} ${month_exec_cash_tot[$sym]} $ratio_tot" >> $cash_monthly_file
        echo "<tr><td><b>$sym</b></td><td>${month_product_cash_ind16[$sym]}</td><td>${month_exec_cash_ind16[$sym]}</td><td>$ratio_ind16</td><td>${month_product_cash_ind17[$sym]}</td><td>${month_exec_cash_ind17[$sym]}</td><td>$ratio_ind17</td><td>${month_product_cash_ind18[$sym]}</td><td>${month_exec_cash_ind18[$sym]}</td><td>$ratio_ind18</td><td>${month_product_cash_tot[$sym]}</td><td>${month_exec_cash_tot[$sym]}</td><td>$ratio_tot</td></tr>" >> $month_cash
  done
  cat $path"daily_ors_report_footer.txt" >> $month_cash
}

init () {

  path="/home/dvcinfra/important/Generate_ors_detail/script/";
  destpath="/var/www/html/ors_report/"
  readpath="/home/dvcinfra/important/Generate_ors_detail/Product_Details/"
  REPORTING_FILE=$destpath"index.html" ;
  chmod +777 $REPORTING_FILE
  monthly_file="${readpath}monthly_data"
  computedDate=$path"already_computed"
  touch $computedDate
  touch $monthly_file
  >$REPORTING_FILE ;

  mkdir -p $destpath ;
  cat $path"generate_ors_header.txt" > $REPORTING_FILE ;
  echo "<table class="table table-striped" style='border: 1px solid grey;margin-bottom: 0px;'><thead><tr>" >> $REPORTING_FILE
  echo "<th>Month</th><th>IND16 Order</th><th>IND16 Exec</th><th>IND16 Ratio</th><th>IND17 Order</th><th>IND17 Exec</th><th>IND17 Ratio</th><th>IND18 Order</th><th>IND18 Exec</th><th>IND18 Ratio</th><th>TotalOrder</th><th>TotalExec</th><th>TotalRatio</th></tr></thead><tbody>" >> $REPORTING_FILE
  yyyymm=`date  +'%Y%m'`
  echo "Month Computing $yyyymm"
  for cashFile in $readpath/* 
  do
      filename=$(basename $cashFile);
      [[ $filename == *"cash" ]] || {  continue; }
      [[ $filename == $yyyymm* ]] || { continue; }
      day=`echo $filename | cut -d'_' -f1`
      while IFS=' ' read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9
      do
          if [[ $f8 == '' ]]; then
              continue;
          fi
          if [ ${month_product_cash_tot[$sym]+_} ]; then
            month_product_cash_ind16[$sym]=$(( ${month_product_cash_ind16[$sym]} + $f1))
            month_exec_cash_ind16[$sym]=$(( ${month_exec_cash_ind16[$sym]} +  $f2))
            month_product_cash_ind17[$sym]=$(( ${month_product_cash_ind17[$sym]} + $f3))
            month_exec_cash_ind17[$sym]=$(( ${month_exec_cash_ind17[$sym]} +  $f4))
            month_product_cash_ind18[$sym]=$(( ${month_product_cash_ind18[$sym]} + $f5))
            month_exec_cash_ind18[$sym]=$(( ${month_exec_cash_ind18[$sym]} +  $f6))
            month_product_cash_tot[$sym]=$(( ${month_product_cash_tot[$sym]} + $f7))
            month_exec_cash_tot[$sym]=$(( ${month_exec_cash_tot[$sym]} +  $f8))
          else
            month_product_cash_ind16[$sym]=$f1
            month_exec_cash_ind16[$sym]=$f2
            month_product_cash_ind17[$sym]=$f3
            month_exec_cash_ind17[$sym]=$f4
            month_product_cash_ind18[$sym]=$f5
            month_exec_cash_ind18[$sym]=$f6
            month_product_cash_tot[$sym]=$f7
            month_exec_cash_tot[$sym]=$f8
          fi
      done <$cashFile
  done
  month_file_gen;
  echo $yyyymm
  grep -q "^$yyyymm" $monthly_file && sed -i "s/^$yyyymm.*/$yyyymm $sum_cash_order_ind16 $sum_cash_exec_ind16 $sum_cash_order_ind17 $sum_cash_exec_ind17 $sum_cash_order_ind18 $sum_cash_exec_ind18 $sum_cash_order_tot $sum_cash_exec_tot /" $monthly_file || echo "$yyyymm $sum_cash_order_ind16 $sum_cash_exec_ind16 $sum_cash_order_ind17 $sum_cash_exec_ind17 $sum_cash_order_ind18 $sum_cash_exec_ind18 $sum_cash_order_tot $sum_cash_exec_tot" >> $monthly_file
  while IFS=' ' read -r f1 f2 f3 f4 f5 f6 f7 f8 f9; do
      echo "<tr><td><b>$f1</b></td><td>$f2<td>$f3</td><td>$((f2 / f3))</td><td>$f4</a></td><td>$f5</td><td>$((f4 / f5))</td><td>$f6</a></td><td>$f7</td><td>$((f6 / f7))</td><td><a href="daily_ors_file/"$f1".cash.index.html" style="color:blue">$f8</a></td><td>$f9</td><td>$((f8 / f9))</td>" >> $REPORTING_FILE
  done < $monthly_file
  echo "</tbody></table>" >> $REPORTING_FILE
  echo "<div class='row header' style='text-align:center;color:green'><h3>ORS REPORTS PER DAY</h3></div>" >> $REPORTING_FILE

  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Date</th><th>CC</th><th>Ind16 Order</th><th>Ind16 Exec</th><th>Ind16 Ratio</th><th>Ind17 Order</th><th>Ind17 Exec</th><th>Ind17 Ratio</th><th>Ind18 Order</th><th>Ind18 Exec</th><th>Ind18 Ratio</th><th>Total Order</th><th>Total Exec</th><th>Total Ratio</th>" >> $REPORTING_FILE;
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
  for cashFile in  $readpath*
  do
    filename=$(basename $cashFile);
    [[ $filename  == *"cash" ]] || {  continue; }
    day=`echo $filename| cut -d'_' -f1`
    echo "<tr><td>$day</td>" >>$REPORTING_FILE;
    mkdir -p $destpath"/daily_ors_file" ;
    sum_cash_order_tot=0;sum_cash_exec_tot=1; sum_cash_order_ind16=0;sum_cash_exec_ind16=1; sum_cash_order_ind17=0;sum_cash_exec_ind17=1; sum_cash_order_ind18=0;sum_cash_exec_ind18=1;
    PerDayORSReportsCash ;
    echo "<td><a href ="daily_ors_file/"$day".cash.index.html" style="color:blue">`cat $cashFile| wc -l`</a></td><td>$sum_cash_order_ind16</td><td>$sum_cash_exec_ind16</td><td>$((sum_cash_order_ind16 / sum_cash_exec_ind16))</td><td>$sum_cash_order_ind17</td><td>$sum_cash_exec_ind17</td><td>$((sum_cash_order_ind17 / sum_cash_exec_ind17))</td><td>$sum_cash_order_ind18</td><td>$sum_cash_exec_ind18</td><td>$((sum_cash_order_ind18 / sum_cash_exec_ind18))</td><td>$sum_cash_order_tot</td><td>$sum_cash_exec_tot</td><td>$((sum_cash_order_tot / sum_cash_exec_tot))</td>" >> $REPORTING_FILE;
  done
  cat $path"generate_ors_footer.txt" >> $REPORTING_FILE;
}

init $*
