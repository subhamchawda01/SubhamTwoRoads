#!/bin/bash

init () {

  path="/home/dvcinfra/important/IND13_DROPS/";
#readpath="/home/dvcinfra/important/ORS_REPLY_INTRADAILY/Product_Details/cash"
  readpath="/tmp/ors_reply_mds_output_file"
  REPORTING_FILE="/tmp/nse_shms_drops_reports.html" ;
  >$REPORTING_FILE ;
  day=`date +"%Y%m%d %H:%M:%S"`;
  cat "/home/pengine/prod/live_configs/generate_ors_header.txt" > $REPORTING_FILE ;
  echo "<h3> DAILY IND13 Drops</h3></div>" >> $REPORTING_FILE ;
  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Date</th><th>Total Drops</th><th>Total Occurance</th><th>FO Occ</th><th>FO Drops</th><th>EQ Occ</th><th>EQ Drops</th></tr></thead><tbody>" >> $REPORTING_FILE;
  cd /home/dvcinfra/important/IND13_DROPS/
  for file in `ls | grep ind13_drop_summary`; do
        date_=`echo $file | cut -d'_' -f4`;
        tot_occ=`grep 'TOTAL DROPS OCCURRENCES:' $file | cut -d' ' -f4`;
        tot_drop=`grep 'TOTAL PKT DROPPED:' $file | cut -d' ' -f4`
        echo "$date_ EX: $tot_occ ET: $tot_drop"
        eq_drop=0
        eq_occ=0
        grep 'IP_PORT' $file | grep EQ >/tmp/f_nse_shm_dorp_temp
        while IFS= read -r line; do
                fd_=`echo $line | cut -d' ' -f3`
                line_=`grep "SOCKET_FD\[$fd_\]" $file`
                eq_occ_temp=`echo $line_ | cut -d' ' -f3`
                eq_drop_temp=`echo $line_ | cut -d' ' -f5`
                eq_occ=$(( eq_occ + eq_occ_temp ))
                eq_drop=$(( eq_drop + eq_drop_temp ))
        done < /tmp/f_nse_shm_dorp_temp
        fo_drop=$(( tot_drop - eq_drop ))
        fo_occ=$(( tot_occ - eq_occ))
        echo "$eq_occ $eq_drop $fo_occ $fo_drop"
        echo "<tr><td><b>$date_</b></td><td>$tot_drop</td><td>$tot_occ</td><td>$fo_occ</td><td>$fo_drop</td><td>$eq_occ</td><td>$eq_drop</td></td></tr>" >> $REPORTING_FILE;
  done

  cat "/home/pengine/prod/live_configs/generate_ors_footer.txt" >> $REPORTING_FILE;
  cp $REPORTING_FILE /var/www/html/nseshm_drops_report/index.html
  chmod +777 /var/www/html/nseshm_drops_report/index.html
}


init $*
