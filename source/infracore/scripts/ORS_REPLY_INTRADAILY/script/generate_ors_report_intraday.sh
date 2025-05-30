#!/bin/bash


init () {

  path="/home/dvcinfra/important/ORS_REPLY_INTRADAILY/script/";
  readpath="/home/dvcinfra/important/ORS_REPLY_INTRADAILY/Product_Details/cash"
  awk -F ' ' '{a[$1] += $2; b[$1] += $3; c[$1] += $4; d[$1] += $5; e[$1] += $6; f[$1] += $7; g[$1] += $8; h[$1] += $9; i[$1] += $10; } END { for (key in a) { print key" "a[key]" "b[key]" "c[key]" "d[key]" "e[key]" "f[key]" "g[key]" "h[key]" "i[key]} }' $readpath >/tmp/intra_cashfile
  REPORTING_FILE="/tmp/intraday_trade_ratio.html" ;
  >$REPORTING_FILE ;
  day=`date +"%Y%m%d"`;
  cat $path"generate_ors_header.txt" > $REPORTING_FILE ;
  echo "<h3>INTRADAY RATIO ORS REPORTS CASH $day</h3></div>" >> $REPORTING_FILE ;
  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Symbol</th><th>IND16 Order</th><th>IND16 Exec</th><th>IND16 Ratio</th><th>IND17 Order</th><th>IND17 Exec</th><th>IND17 Ratio</th><th>IND18 Order</th><th>IND18 Exec</th><th>IND18 Ratio</th><th>TotalOrder</th><th>TotalExec</th><th>TotalRatio</th><th>Price</th></tr></thead><tbody>" >> $REPORTING_FILE;
  while IFS=' ' read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9
  do
        ratio_ind16=-1
        [[ $f2 -eq 0 ]] || ratio_ind16=$(( f1 / f2 ))
        ratio_ind17=-1
        [[ $f4 -eq 0 ]] || ratio_ind17=$(( f3 / f4 ))
        ratio_ind18=-1
        [[ $f6 -eq 0 ]] || ratio_ind18=$(( f5 / f6 ))
        ratio_tot=-1
	[[ $f8 -eq 0 ]] || ratio_tot=$(( f7 / f8 ))
        echo "<tr><td><b>$sym</b></td><td>$f1</td><td>$f2</td><td>$ratio_ind16</td><td>$f3</td><td>$f4</td><td>$ratio_ind17</td><td>$f5</td><td>$f6</td><td>$ratio_ind18</td><td>$f7</td><td>$f8</td><td>$ratio_tot</td><td>$f9</td></tr>" >> $REPORTING_FILE;
  done </tmp/intra_cashfile

  cat $path"generate_ors_footer.txt" >> $REPORTING_FILE;
  cp $REPORTING_FILE /var/www/html/intraday_trade_ratio/index.html
  chmod +777 /var/www/html/intraday_trade_ratio/index.html
}

init $*
