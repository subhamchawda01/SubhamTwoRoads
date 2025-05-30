#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

init () {

  [ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < DATE >" ;
  
  yyyymmdd=$1; 
  MODE=$2 
  if [ "$1" == "TODAY" ] ; then 
    yyyymmdd=`date +"%Y%m%d"` ;
  fi  

  yyyy=${yyyymmdd:0:4};
  mm=${yyyymmdd:4:2};  
  dd=${yyyymmdd:6:2};

  REPORTING_DIR="/var/www/html/LatencyReportsExchange/$yyyymmdd" ;
  rm -rf $REPORTING_DIR;
  mkdir -p $REPORTING_DIR ;

  REPORTING_FILE="/var/www/html/LatencyReportsExchange/index.html" ;
  >$REPORTING_FILE ;

  dttime=`date +"%Y%m%d.%H%M"`;

  #HEADER
  cat /home/pengine/prod//live_configs/latency_report_header.txt | sed 's/LATENCY REPORTS/EXCHANGE LATENCY REPORTS [ '$dttime' ] /g' >> $REPORTING_FILE ; 

#  if [ $MODE == "INTRADAY" ]; then
#    REPORT_EXEC=/home/pengine/prod/live_execs/ors_binary_reader_stats_exchange_intraday ;
#  elif [ $MODE == "NORMAL" ]; then
    REPORT_EXEC=/home/pengine/prod/live_execs/ors_binary_reader_stats_exchange ;
#  fi
  echo "MODE : $MODE"
  echo "EXEC : $REPORT_EXEC"
  REPORT_SERVER_CONFIG=/home/pengine/prod/live_configs/server_latency_reporting_config.txt ;

  REPORT_SERVER_HTML_ALL_DATA="/var/www/html/LatencyReportsExchange/exchange_historical_records.txt" ;

  [ -f $REPORT_EXEC ] || print_msg_and_exit "REPORT_EXEC -> $REPORT_EXEC DOESN'T EXIST" ;


#  if [ $MODE == "INTRADAY" ]; then
    fcount=`ls /NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/$yyyy/$mm/$dd | wc -l`;
#  elif [ $MODE == "NORMAL" ]; then
#    fcount=`ls /NAS1/data/ORSData/NSE/$yyyy/$mm/$dd/* | wc -l`;
#  fi
  [ $fcount -gt 0 ] || print_msg_and_exit "NO DATAFILES PRESENT FOR REPORTING";

  echo "<th>DATE</th>" >> $REPORTING_FILE ;

  servers="INDB12" ;

  for server in $servers; 
  do
    echo "<th><a href=$server".index.html" style="color:red">$server</a></th>" >> $REPORTING_FILE ; 
  done 

  echo "</tr></thead><tbody>" >> $REPORTING_FILE ; 

  grep -v "$yyyymmdd" $REPORT_SERVER_HTML_ALL_DATA | sort -t$'>' -nrk 3 > /tmp/exchange_latency_report.txt ;
  mv /tmp/exchange_latency_report.txt $REPORT_SERVER_HTML_ALL_DATA ;
  
  echo "1EXCH NUMBER OF LINES $REPORT_SERVER_HTML_ALL_DATA" `wc -l $REPORT_SERVER_HTML_ALL_DATA` ;


  lineprint="<tr><td>$yyyymmdd</td>";

  for server in $servers;
  do

     BASE_SACI=`grep -w "$server" $REPORT_SERVER_CONFIG | awk '{print $2}'` ;
     FREQ=`grep -w "$server" $REPORT_SERVER_CONFIG | awk '{print $3}'` ;
     RACK=`grep -w "$server" $REPORT_SERVER_CONFIG | awk '{print $4}'` ;

     cd $REPORTING_DIR;
#     if [ $MODE == "INTRADAY" ]; then
       echo "$MODE : $REPORT_EXEC $server $BASE_SACI $FREQ $yyyymmdd find /NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/$yyyy/$mm/$dd"
       $REPORT_EXEC $server $BASE_SACI $FREQ $yyyymmdd live `find /NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/$yyyy/$mm/$dd | sed 's/\&/\\&/g'` ;   
#     elif [ $MODE == "NORMAL" ]; then
#       echo "$MODE : $REPORT_EXEC $server $BASE_SACI $FREQ $yyyymmdd ls /NAS1/data/ORSData/NSE/$yyyy/$mm/$dd/* | sed 's/\&/\\&/g'" ;
#       $REPORT_EXEC $server $BASE_SACI $FREQ $yyyymmdd `ls /NAS1/data/ORSData/NSE/$yyyy/$mm/$dd/* | sed 's/\&/\\&/g'` ;
#     fi

     REPORT_SERVER_WISE_HTML_ALL_DATA="/var/www/html/LatencyReportsExchange/"$server"_exchange_historical_records.txt" ;
     grep -v "$yyyymmdd" $REPORT_SERVER_WISE_HTML_ALL_DATA | sort -t$'>' -nrk 3 > /tmp/exchange_latency_report.txt
     mv /tmp/exchange_latency_report.txt $REPORT_SERVER_WISE_HTML_ALL_DATA ;

     echo "2EXCH NUMBER OF LINES $REPORT_SERVER_WISE_HTML_ALL_DATA" `wc -l $REPORT_SERVER_HTML_ALL_DATA` ;

     SERVER_REPORTING_FILE=$server"_"$yyyymmdd"_exchange_latency_report.html";
     SERVER_REPORTING_FILE_LINK="$yyyymmdd/$SERVER_REPORTING_FILE" ;
     >$SERVER_REPORTING_FILE;

     SERVER_ALL_DATA_REPORT_FILE="/var/www/html/LatencyReportsExchange/"$server".index.html" ;
     >$SERVER_ALL_DATA_REPORT_FILE ;
     cat /home/pengine/prod//live_configs/latency_report_header.txt | sed 's/LATENCY REPORTS/'$server' EXCHANGE LATENCY REPORTS/g' >> $SERVER_ALL_DATA_REPORT_FILE ;

     cat /home/pengine/prod//live_configs/latency_report_header.txt | sed 's/LATENCY REPORTS/'$server' EXCHANGE LATENCY REPORTS FOR '$yyyymmdd'/g' >> $SERVER_REPORTING_FILE ;
     echo "<th>DATE</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>MIN</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>1%</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>10%</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>25%</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>50%</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>75%</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>90%</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>99%</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>MAX</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>AVG</th>" >> $SERVER_REPORTING_FILE ;
     echo "<th>SAMPLE</th>" >> $SERVER_REPORTING_FILE ;
     echo "</tr></thead><tbody>" >> $SERVER_REPORTING_FILE ;

     SERVER_GNUPLOT_FILE=$server"_"$yyyymmdd".jpeg" ;

     echo "<th>DATE</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>Graph</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>Products</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>MIN</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>1%</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>10%</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>25%</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>50%</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>75%</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>90%</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>99%</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>MAX</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>AVG</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "<th>SAMPLE</th>" >> $SERVER_ALL_DATA_REPORT_FILE ;
     echo "</tr></thead><tbody>" >> $SERVER_ALL_DATA_REPORT_FILE ;

     SERVER_TIMELY_FILE=$server"_"$yyyymmdd"_exchange_timely_latency.txt"

     GNUPLOT='set terminal jpeg size 1366,768; set output "'$SERVER_GNUPLOT_FILE'"; set title "'$server'->'$yyyymmdd'"; set xlabel "Time ( UTC )"; set ylabel "Latency(Micros)"; set xdata time; set timefmt "%s"; plot "'$SERVER_TIMELY_FILE'" using 1:4 title "10%" with lines, "'$SERVER_TIMELY_FILE'" using 1:6 title "Median" with lines, "'$SERVER_TIMELY_FILE'" using 1:8 title "90%" with lines, "'$SERVER_TIMELY_FILE'" using 1:11 title "Mean" with lines';

     echo $GNUPLOT | gnuplot ;

#PRODUCT WISE DETAILS     

     PRODUCT_WISE_REPORT_FILE=$server"_"$yyyymmdd"_products_exchange_latency_report.html";
     PRODUCT_WISE_REPORT_FILE_LINK="$yyyymmdd/$PRODUCT_WISE_REPORT_FILE" ;

     >$PRODUCT_WISE_REPORT_FILE;
     cat /home/pengine/prod//live_configs/latency_report_header.txt | sed 's/LATENCY REPORTS/'$server' PRODUCT WISE EXCHANGE LATENCY REPORTS FOR '$yyyymmdd'/g' >> $PRODUCT_WISE_REPORT_FILE ;

     echo "<th>PRODUCT</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>Graph</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>MIN</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>1%</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>10%</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>25%</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>50%</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>75%</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>90%</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>99%</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>MAX</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>AVG</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "<th>SAMPLE</th>" >> $PRODUCT_WISE_REPORT_FILE ;
     echo "</tr></thead><tbody>" >> $PRODUCT_WISE_REPORT_FILE ;

     for files in `ls $server*_BSE*exchange_overall_latency.txt`; do

       product=`echo $files | awk -F"$server" '{print $2}' | awk -F"_$yyyymmdd" '{print $1}' | awk -F"_BSE" '{print "BSE"$2}'`;

       PRODUCT_GNUPLOT_FILE=$server"_"$yyyymmdd"_"$product".jpeg" ;
       PRODUCT_TIMELY_FILE=`echo $files | sed 's/overall/timely/g'`;

       GNUPLOT='set terminal jpeg size 1366,768; set output "'$PRODUCT_GNUPLOT_FILE'"; set title "'$server'->'$yyyymmdd'"; set xlabel "Time ( UTC )"; set ylabel "Latency(Micros)"; set xdata time; set timefmt "%s"; plot "'$PRODUCT_TIMELY_FILE'" using 1:4 title "10%" with lines, "'$PRODUCT_TIMELY_FILE'" using 1:6 title "Median" with lines, "'$PRODUCT_TIMELY_FILE'" using 1:8 title "90%" with lines, "'$PRODUCT_TIMELY_FILE'" using 1:11 title "Mean" with lines';

       echo $GNUPLOT | gnuplot ;

#PRODUCT
       serverlineprint="<tr><td>$product</td><td><a href=$PRODUCT_GNUPLOT_FILE><img border="0" src="../Graph-512.png" width="40" height="25"></a></td>";

       for data in `cat $files`; do
         serverlineprint=`echo $serverlineprint"<td>"$data"</td>"` ;
       done 

       serverlineprint=`echo $serverlineprint"</tr>"`;
       echo $serverlineprint >> $PRODUCT_WISE_REPORT_FILE ;

     done

     cat /home/pengine/prod//live_configs/latency_report_footer.txt >> $PRODUCT_WISE_REPORT_FILE ;

#OVERALL ALL DATA
     serverlineprint="<tr><td>$yyyymmdd</td><td><a href=$yyyymmdd/$SERVER_GNUPLOT_FILE><img border="0" src="Graph-512.png" width="40" height="25"></a></td><td><a href=$PRODUCT_WISE_REPORT_FILE_LINK style="color:blue"><img border="0" alt="Products" src="chart_3.png" width="40" height="25"></a></td>" ; 

     for data in `cat $server"_"$yyyymmdd"_exchange_overall_latency.txt"` ; do
       serverlineprint=`echo $serverlineprint"<td>"$data"</td>"` ;
     done 

     serverlineprint=`echo $serverlineprint"</tr>"`;
     echo $serverlineprint >> $SERVER_ALL_DATA_REPORT_FILE ;
     cat $REPORT_SERVER_WISE_HTML_ALL_DATA >> $SERVER_ALL_DATA_REPORT_FILE ;
     cat /home/pengine/prod//live_configs/latency_report_footer.txt >> $SERVER_ALL_DATA_REPORT_FILE ;

     echo $serverlineprint >> $REPORT_SERVER_WISE_HTML_ALL_DATA ;

#OVERALL
     serverlineprint="<tr><td>$yyyymmdd[OVERALL]</td>";

     for data in `cat $server"_"$yyyymmdd"_exchange_overall_latency.txt"` ; do
       serverlineprint=`echo $serverlineprint"<td>"$data"</td>"` ;
     done 

     serverlineprint=`echo $serverlineprint"</tr>"`;
     echo $serverlineprint >> $SERVER_REPORTING_FILE; 

#SAMECT
     serverlineprint="<tr><td>$yyyymmdd[SAMECT]</td>";

     for data in `cat $server"_"$yyyymmdd"_exchange_samect_latency.txt"` ; do
       serverlineprint=`echo $serverlineprint"<td>"$data"</td>"` ;
     done 

     serverlineprint=`echo $serverlineprint"</tr>"`;
     echo $serverlineprint >> $SERVER_REPORTING_FILE; 

#SEQ     
     serverlineprint="<tr><td>$yyyymmdd[SEQ]</td>";

     for data in `cat $server"_"$yyyymmdd"_exchange_seq_latency.txt"` ; do
       serverlineprint=`echo $serverlineprint"<td>"$data"</td>"` ;
     done 

     serverlineprint=`echo $serverlineprint"</tr>"`;
     echo $serverlineprint >> $SERVER_REPORTING_FILE; 

#CXLSEQ     
     serverlineprint="<tr><td>$yyyymmdd[CXLSEQ]</td>";

     for data in `cat $server"_"$yyyymmdd"_exchange_cxlseq_latency.txt"` ; do
       serverlineprint=`echo $serverlineprint"<td>"$data"</td>"` ;
     done 

     serverlineprint=`echo $serverlineprint"</tr>"`;
     echo $serverlineprint >> $SERVER_REPORTING_FILE; 

#MODSEQ     
     serverlineprint="<tr><td>$yyyymmdd[MODSEQ]</td>";

     for data in `cat $server"_"$yyyymmdd"_exchange_modifyseq_latency.txt"` ; do
       serverlineprint=`echo $serverlineprint"<td>"$data"</td>"` ;
     done 

     serverlineprint=`echo $serverlineprint"</tr>"`;
     echo $serverlineprint >> $SERVER_REPORTING_FILE; 
     cat /home/pengine/prod//live_configs/latency_report_footer.txt >> $SERVER_REPORTING_FILE ;

     latencyvalue=`cat $server"_"$yyyymmdd"_exchange_overall_latency.txt" | awk '{print $5}'` ;
     lineprint=`echo $lineprint"<td><a href="$SERVER_REPORTING_FILE_LINK">"$latencyvalue"</a></td>"` ;

  done

  lineprint=`echo $lineprint"</tr>"` ;
 
  echo $lineprint >> $REPORTING_FILE ;
  #Add Older Data
  cat $REPORT_SERVER_HTML_ALL_DATA >> $REPORTING_FILE ; 
  cat /home/pengine/prod//live_configs/latency_report_footer.txt >> $REPORTING_FILE ;
  echo $lineprint >> $REPORT_SERVER_HTML_ALL_DATA ;

}

init $*
