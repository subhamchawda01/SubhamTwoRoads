#!/bin/bash

init () {
    path="/home/dvcinfra/important/BSE_IND11_DROPS/";
    REPORTING_FILE="/tmp/bse_shms_drops_reports.html" ;
    >$REPORTING_FILE ;
    day=`date +"%Y%m%d %H:%M:%S"`;
    cat "/home/dvcinfra/important/BSE_IND11_DROPS/generate_ors_header.txt" > $REPORTING_FILE ;
    echo "<h3> DAILY BSE INDB11 Drops</h3></div>" >> $REPORTING_FILE ;
    echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
    echo "<th>Date</th><th>Total Drops</th><th>Total Occurance</th><th>FO Occ</th><th>FO Drops</th><th>EQ Occ</th><th>EQ Drops</th></tr></thead><tbody>" >> $REPORTING_FILE;
    cd /home/dvcinfra/important/BSE_IND11_DROPS/
    for file in `ls | grep ind11_drop_summary_`; do
        date_=`echo $file | cut -d'_' -f4`;
#       tot_occ=`grep 'TOTAL DROPS OCCURRENCES:' $file | cut -d' ' -f4`;
#       tot_drop=`grep 'TOTAL PKT DROPPED:' $file | cut -d' ' -f4`
#        echo "$date_ EX: $tot_occ ET: $tot_drop"
        fo_occ=`grep 'TOTAL NSE DROPS OCCURRENCES:' $file | cut -d' ' -f5`;
        fo_drop=`grep 'TOTAL NSE PKT DROPPED:' $file | cut -d' ' -f5`;

        eq_occ=`grep 'TOTAL BSE DROPS OCCURRENCES:' $file | cut -d' ' -f5`;
        eq_drop=`grep 'TOTAL BSE PKT DROPPED:' $file | cut -d' ' -f5`;

        tot_occ=$((fo_occ + eq_occ))
        tot_drop=$((fo_drop + eq_drop))
        echo "$date_ EX: $tot_occ ET: $tot_drop"

#       grep 'IP_PORT' $file | grep FO >/tmp/f_bse_shm_dorp_temp
#       while IFS= read -r line; do
#           echo "21123"
#           fd_=`echo $line | cut -d' ' -f3`
#             echo "213"
#        line_=`grep "SOCKET_FD\[$fd_\]" $file`
#           echo "1"
#           fo_occ_temp=`echo $line_ | cut -d' ' -f3`
#           echo "12"
#           fo_drop_temp=`echo $line_ | cut -d' ' -f5`
#           fo_occ=$(( fo_occ + fo_occ_temp ))
#           fo_drop=$(( fo_drop + fo_drop_temp ))
#           echo "2134"
#       done < /tmp/f_bse_shm_dorp_temp
#       echo "DROP $tot_drop FO: $fo_drop OCC: $tot_occ"
#       eq_drop=$(( tot_drop - fo_drop ))
#       echo "DROP $fo_drop"
#        eq_occ=$(( tot_occ - fo_occ))
        echo "EQ OCC:$eq_occ EQ DROP:$eq_drop FO OCC:$fo_occ FO DROP:$fo_drop"
        echo "<tr><td><b>$date_</b></td><td>$tot_drop</td><td>$tot_occ</td><td>$fo_occ</td><td>$fo_drop</td><td>$eq_occ</td><td>$eq_drop</td></td></tr>" >> $REPORTING_FILE;
    done
    
    cat "/home/dvcinfra/important/BSE_IND11_DROPS/generate_ors_footer.txt" >> $REPORTING_FILE;
    cp $REPORTING_FILE /var/www/html/bseshm_drops_report/index.html
    chmod +777 /var/www/html/bseshm_drops_report/index.html
}


init $*

