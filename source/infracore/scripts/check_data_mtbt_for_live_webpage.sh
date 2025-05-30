#!/bin/bash

init () {

  REPORTING_FILE="/tmp/mtbt_shm_data_reports.html" ;
  >$REPORTING_FILE ;
  day=`date +"%Y%m%d %H:%M:%S"`;
  cat "/home/pengine/prod/live_configs/generate_live_mtbt_header.txt" > $REPORTING_FILE ;
  echo "<h3> Live Product Missing MTBT </h3></div>" >> $REPORTING_FILE ;
  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Machine</th><th>Products</th><th>Symbol</th></tr></thead><tbody>" >> $REPORTING_FILE;
  while IFS= read -r line; do
    machine_=`echo $line | cut -d' ' -f1`
    product=`echo $line | cut -d' ' -f2`
    file_=`echo $line | cut -d' ' -f3`
    echo $product
    echo "<tr><td><b>$machine_</b></td><td>$product</td><td>$file_</td></tr>" >> $REPORTING_FILE;
  done < /home/dvcinfra/important/live_prod_no_data.txt

  cat "/home/pengine/prod/live_configs/generate_ors_footer.txt" >> $REPORTING_FILE;
  cp $REPORTING_FILE /var/www/html/live_mtbt_data_missing/index.html
  chmod +777 /var/www/html/live_mtbt_data_missing/index.html
}


init $*
