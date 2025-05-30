#!/bin/bash
declare -A month_product_cash
declare -A month_exec_cash
declare -A month_product_fut
declare -A month_exec_fut
declare -A month_product_opt
declare -A month_exec_opt

print_msg_and_exit () {
  echo $* ;
  exit ;
}

PerDayORSReportsCash () { 
  PerDayORS_cash=$destpath"daily_ors_file/"$day".cash.index.html"
  > $PerDayORS_cash ;
  chmod +777 $PerDayORS_cash
  cat $path"daily_ors_report_header.txt" | sed 's/ORS REPORTS/ORS REPORTS [ '$day' ] /g' >> $PerDayORS_cash ;
  echo "<th>Symbol</th><th>Total</th><th>Exec</th><th>Ratio</th></tr></thead><tbody>" >> $PerDayORS_cash
  while read line
  do
         sym=`echo $line | cut -d' ' -f 1`
         tot=`echo $line | cut -d' ' -f 2`
         exec_=`echo $line | cut -d' ' -f 3`
	 ratio=`echo $line | cut -d' ' -f 4`
         sum_cash_tot=$(( $sum_cash_tot + $tot ))
         sum_cash_exec=$(( $sum_cash_exec + $exec_ ))
         echo "<tr><td><b>$sym</b></td><td>$tot</td><td>$exec_</td><td>$ratio</td></tr>" >> $PerDayORS_cash
  done < $cashFile
  cat $path"daily_ors_report_footer.txt" >> $PerDayORS_cash
}

PerDayORSReportsFut () {
  PerDayORS_fut=$destpath"daily_ors_file/"$day".fut.index.html"
  > $PerDayORS_fut ;
  chmod +777 $PerDayORS_fut
  cat $path"daily_ors_report_header.txt" | sed 's/ORS REPORTS/ORS REPORTS [ '$day' ] /g' >> $PerDayORS_fut ;
  echo "<th>Symbol</th><th>Total</th><th>Exec</th><th>Ratio</th></tr></thead><tbody>" >> $PerDayORS_fut
  while read line
  do
      sym=`echo $line | cut -d' ' -f 1`
      tot=`echo $line | cut -d' ' -f 2`
      exec_=`echo $line | cut -d' ' -f 3`
      ratio=`echo $line | cut -d' ' -f 4`
      sum_fut_tot=$(( $sum_fut_tot + $tot ))
      sum_fut_exec=$(( $sum_fut_exec + $exec_ ))
      echo "<tr><td><b>$sym</b></td><td>$tot</td><td>$exec_</td><td>$ratio</td></tr>" >> $PerDayORS_fut
  done < $futFile
  cat $path"daily_ors_report_footer.txt" >> $PerDayORS_fut
}

PerDayORSReportsOpt () {
  PerDayORS_opt=$destpath"daily_ors_file/"$day".opt.index.html"
  > $PerDayORS_opt ;
  chmod +777 $PerDayORS_opt
  cat $path"daily_ors_report_header.txt" | sed 's/ORS REPORTS/ORS REPORTS [ '$day' ] /g' >> $PerDayORS_opt ;
  echo "<th>Symbol</th><th>Total</th><th>Exec</th><th>Ratio</th></tr></thead><tbody>" >> $PerDayORS_opt
  while read line
  do
         sym=`echo $line | cut -d' ' -f 1`
         tot=`echo $line | cut -d' ' -f 2`
         exec_=`echo $line | cut -d' ' -f 3`
         ratio=`echo $line | cut -d' ' -f 4`
         sum_opt_tot=$(( $sum_opt_tot + $tot ))
         sum_opt_exec=$(( sum_opt_exec + $exec_ ))
         echo "<tr><td><b>$sym</b></td><td>$tot</td><td>$exec_</td><td>$ratio</td></tr>" >> $PerDayORS_opt
  done < $optFile
  cat $path"daily_ors_report_footer.txt" >> $PerDayORS_opt
}


month_file_gen(){
  month_cash=$destpath"daily_ors_file/"$yyyymm".cash.index.html"
  month_fut=$destpath"daily_ors_file/"$yyyymm".fut.index.html"
  month_opt=$destpath"daily_ors_file/"$yyyymm".opt.index.html"
  >$month_cash; >$month_fut; >$month_opt;
  chmod +777 $month_cash; chmod +777 $month_fut; chmod +777 $month_opt;
  cat $path"daily_ors_report_header.txt" | sed "s/ORS REPORTS/ORS REPORTS MONTHLY ${yyyymm} CASH /g" >> $month_cash ;
  echo "<th>Symbol</th><th>Total</th><th>Exec</th><th>Ratio</th></tr></thead><tbody>" >> $month_cash ;
  cat $path"daily_ors_report_header.txt" | sed "s/ORS REPORTS/ORS REPORTS MONTHLY ${yyyymm} FUT  /g" >> $month_fut ;
  echo "<th>Symbol</th><th>Total</th><th>Exec</th><th>Ratio</th></tr></thead><tbody>" >> $month_fut ;
  cat $path"daily_ors_report_header.txt" | sed "s/ORS REPORTS/ORS REPORTS MONTHLY ${yyyymm} OPTION  /g" >> $month_opt ;
  echo "<th>Symbol</th><th>Total</th><th>Exec</th><th>Ratio</th></tr></thead><tbody>" >> $month_opt ;
  sum_cash_tot=0;sum_cash_exec=0;sum_fut_tot=0;sum_fut_exec=0;sum_opt_tot=0;sum_opt_exec=0;
  for sym in "${!month_product_cash[@]}"; do
        ratio=-1
        sum_cash_tot=$(( $sum_cash_tot + ${month_product_cash[$sym]} ))
        sum_cash_exec=$(( $sum_cash_exec + ${month_exec_cash[$sym]} ))
        [[ ${month_exec_cash[$sym]} == 0 ]] || ratio=$((${month_product_cash[$sym]} / ${month_exec_cash[$sym]}))
        echo "<tr><td><b>$sym</b></td><td>${month_product_cash[$sym]}</td><td>${month_exec_cash[$sym]}</td><td>$ratio</td></tr>" >> $month_cash
  done
  for sym in "${!month_product_fut[@]}"; do
        ratio=-1
        sum_fut_tot=$(( $sum_fut_tot + ${month_product_fut[$sym]} ))
        sum_fut_exec=$(( $sum_fut_exec + ${month_exec_fut[$sym]} ))
        [[ ${month_exec_fut[$sym]} == 0 ]]  || ratio=$((${month_product_fut[$sym]} / ${month_exec_fut[$sym]}))
        echo "<tr><td><b>$sym</b></td><td>${month_product_fut[$sym]}</td><td>${month_exec_fut[$sym]}</td><td>$ratio</td></tr>" >> $month_fut
  done
  for sym in "${!month_product_opt[@]}"; do
        ratio=-1
        sum_opt_tot=$(( $sum_opt_tot + ${month_product_opt[$sym]} ))
        sum_opt_exec=$(( $sum_opt_exec + ${month_exec_opt[$sym]} ))
        [[ ${month_exec_opt[$sym]} == 0 ]] || ratio=$((${month_product_opt[$sym]} / ${month_exec_opt[$sym]}))
        echo "<tr><td><b>$sym</b></td><td>${month_product_opt[$sym]}</td><td>${month_exec_opt[$sym]}</td><td>$ratio</td></tr>" >> $month_opt
  done
  
  cat $path"daily_ors_report_footer.txt" >> $month_cash
  cat $path"daily_ors_report_footer.txt" >> $month_fut
  cat $path"daily_ors_report_footer.txt" >> $month_opt
}

init () {

  path="/home/dvcinfra/important/Generate_ors_detail/script/";
  destpath="/var/www/html/ors_report/"
  readpath="/home/dvcinfra/important/Generate_ors_detail/Product_Details/"
  REPORTING_FILE=$destpath"index.html" ;
  chmod +777 $REPORTING_FILE
  monthly_file="${readpath}monthly_data"
  touch $monthly_file
  >$REPORTING_FILE ;

  mkdir -p $destpath ;
  cat $path"generate_ors_header.txt" > $REPORTING_FILE ;
  echo "<table class="table table-striped" style='border: 1px solid grey;margin-bottom: 0px;'><thead><tr>" >> $REPORTING_FILE
  echo "<th>Month</th><th>Total Cash</th><th>Exec Cash</th><th>%</th><th>Total Fut</th><th>Exec Fut</th><th>%</th><th>Total Opt</th><th>Exec Opt</th><th>%</th></tr></thead><tbody>" >> $REPORTING_FILE
  yyyymm=`date  +'%Y%m'`
  yyyymm=202001
  echo "Month Computing $yyyymm"
  for file in  $readpath*
  do
      filename=$(basename $file);
      [[ $filename == "$yyyymm"*"_cash" ]] || continue ;
      day=`echo $filename| cut -d'_' -f1`
      cashFile=$file
      futFile="$readpath${day}_fut"
      optFile="$readpath${day}_opt"
      while IFS=' ' read -r sym f1 f2 f3 
      do
          if [ ${month_product_cash[$sym]+_} ]; then
            month_product_cash[$sym]=$(( ${month_product_cash[$sym]} + $f1))
            month_exec_cash[$sym]=$(( ${month_exec_cash[$sym]} +  $f2))
          else
            month_product_cash[$sym]=$f1
            month_exec_cash[$sym]=$f2
          fi
      done <$cashFile
      while IFS=' ' read -r sym f1 f2 f3 
      do
           if [ ${month_product_fut[$sym]+_} ]; then
             month_product_fut[$sym]=$(( ${month_product_fut[$sym]} + $f1))
             month_exec_fut[$sym]=$(( ${month_exec_fut[$sym]} + $f2))
           else
             month_product_fut[$sym]=$f1
             month_exec_fut[$sym]=$f2
           fi
      done <$futFile
      while IFS=' ' read -r sym f1 f2 f3 
      do
          if [ ${month_product_opt[$sym]+_} ]; then
            month_product_opt[$sym]=$(( ${month_product_opt[$sym]} + $f1)) 
            month_exec_opt[$sym]=$(( ${month_exec_opt[$sym]} + $f2))
          else
            month_product_opt[$sym]=$f1
            month_exec_opt[$sym]=$f2
          fi
      done <$optFile
  done
  month_file_gen;
  echo $yyyymm
  grep -q "^$yyyymm" $monthly_file && sed -i "s/^$yyyymm.*/$yyyymm $sum_cash_tot $sum_cash_exec $sum_fut_tot $sum_fut_exec $sum_opt_tot $sum_opt_exec/" $monthly_file || echo "$yyyymm $sum_cash_tot $sum_cash_exec $sum_fut_tot $sum_fut_exec $sum_opt_tot $sum_opt_exec" >> $monthly_file
  while IFS=' ' read -r f1 f2 f3 f4 f5 f6 f7; do
      echo "<tr><td><b>$f1</b></td><td><a href="daily_ors_file/"$f1".cash.index.html" style="color:blue">$f2</a></td><td>$f3</td><td>$((f2 / f3))</td>" >> $REPORTING_FILE
      echo "<td><a href="daily_ors_file/"$f1".fut.index.html" style="color:blue">$f4</a></td><td>$f5</td><td>$((f4 / f5))</td>" >>$REPORTING_FILE
      echo "<td><a href="daily_ors_file/"$f1".opt.index.html" style="color:blue">$f6</a></td><td>$f7</td><td>$((f6 / f7))</td></tr>" >> $REPORTING_FILE
  done < $monthly_file
  echo "</tbody></table>" >> $REPORTING_FILE
  echo "<div class='row header' style='text-align:center;color:green'><h3>ORS REPORTS PER DAY</h3></div>" >> $REPORTING_FILE

  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Date</th><th>CC</th><th>Total Cash</th><th>Exec Cash</th><th>%</th><th>FC</th><th>Total Fut</th><th>Exec Fut</th><th>%</th><th>OC</th><th>Total Opt</th><th>Exec Opt</th><th>%</th>" >> $REPORTING_FILE;
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
  for file in $readpath*
  do
    filename=$(basename $file);
    [[ $filename == *"cash" ]] || continue;
    day=`echo $filename| cut -d'_' -f1`
    echo "<tr><td>$day</td>" >>$REPORTING_FILE;
    mkdir -p $destpath"/daily_ors_file" ;
    cashFile=$file
    futFile="$readpath/${day}_fut"
    optFile="$readpath/${day}_opt"
    sum_cash_tot=0;sum_cash_exec=0;
    sum_fut_tot=0;sum_fut_exec=0;
    sum_opt_tot=0;sum_opt_exec=0;
    PerDayORSReportsCash ;
    PerDayORSReportsFut ;
    PerDayORSReportsOpt ; 
    echo "<td><a href ="daily_ors_file/"$day".cash.index.html" style="color:blue">`cat $cashFile| wc -l`</a></td><td>$sum_cash_tot</td><td>$sum_cash_exec</td><td>$((sum_cash_tot / sum_cash_exec))</td>" >> $REPORTING_FILE;
    echo "<td><a href ="daily_ors_file/"$day".fut.index.html" style="color:blue">`cat $futFile| wc -l`</a></td><td>$sum_fut_tot</td><td>$sum_fut_exec</td><td>$((sum_fut_tot / sum_fut_exec))</td>" >> $REPORTING_FILE;
    echo "<td><a href ="daily_ors_file/"$day".opt.index.html" style="color:blue">`cat $optFile| wc -l`</a></td><td>$sum_opt_tot</td><td>$sum_opt_exec</td><td>$((sum_opt_tot / sum_opt_exec))</td></tr>" >> $REPORTING_FILE;
  done
  cat $path"generate_ors_footer.txt" >> $REPORTING_FILE;
}

init $*

