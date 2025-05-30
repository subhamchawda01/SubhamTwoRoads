#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

compute_and_send_ind_rtts () {

  [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < TODAY >" ;

  ORS_BIN_EXEC="/home/pengine/prod/live_execs/ors_binary_reader" ;
  [ -f $ORS_BIN_EXEC ] || print_msg_and_exit "ORS_BIN_EXEC -> $ORS_BIN_EXEC DOESN't EXIST" ;

  YYYYMMDD=$1 ;

  if [ "$1" == "TODAY" ] ; then 
    YYYYMMDD=`date +%"Y%m%d"` ; 
  fi 

  TOTAL_NSE_FILES=`ls /NAS1/data/ORSData/*/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/* | wc -l` ; 

  if [ $TOTAL_NSE_FILES -le 0 ] ; then 
    print_msg_and_exit "THERE ARE NO DATAFILES FOR : $YYYYMMDD TO GENERATE RTTS STATS" ; 
  fi   

  TEMP_FILE="/tmp/rtts_summary.txt" ; 
  >$TEMP_FILE ; 

  printf "Subject: ORS RTTS FOR : $YYYYMMDD \n" >> $TEMP_FILE ; 
  printf "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n\n" >> $TEMP_FILE ; 
  printf "<html><body>\n" >> $TEMP_FILE ; 

  DATFILE=/spare/local/RTTDatabase/$YYYYMMDD".dat" ;
  REFDB="/spare/local/RTTDatabase/rtts_reference.db" ; 

  >$DATFILE ;

  for loc in `ls /NAS1/data/ORSData/*/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/* | awk -F"/" '{print $5}' | sort | uniq` 
  do 

    median_latency=0 ; 
    high_latency=0 ;
    count=0 ;

    total_median=`grep "$loc" $REFDB | tail -n 50 | tr ':' ' ' | awk '{print $6}' | xargs | tr ' ' '+' | bc` ; 
    total_high=`grep "$loc" $REFDB | tail -n 50 | tr ':' ' ' | awk '{print $8}' | xargs | tr ' ' '+' | bc` ;
    total_count=`grep "$loc" $REFDB | tail -n 50 | wc -l` ; 

    benchmark_median=`echo $total_median" "$total_count | awk '{printf "%d", ($1/$2)}'` ;  
    benchmark_high=`echo $total_high" "$total_count | awk '{printf "%d", ($1/$2)}'` ;
    compare_benchmark_median=`echo $benchmark_median | awk '{printf "%d", ($1*1.2)}'` ; 
    compare_benchmark_high=`echo $benchmark_high | awk '{printf "%d", ($1*1.2)}'` ; 

    printf "<table border = \"1\"><tr>" >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"5\" color=#424242>%s</font></td></tr>" $loc >> $TEMP_FILE ; 
#    printf "<td align=center><font font-weight = \"bold\" size = \"5\" color=#424242>%s</font></td>" "AVG_MED -> $compare_benchmark_median" >> $TEMP_FILE ; 
#    printf "<td align=center><font font-weight = \"bold\" size = \"5\" color=#424242>%s</font></td></tr>" "AVG_95 -> $compare_benchmark_high" >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "PRODUCT" >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "SAMPLE_SIZE" >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "AVERAGE" >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "AVG_HIST_MEDIAN" >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" " MEDIAN " >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "  90%TILE  " >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "AVG_HIST_95" >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "  95%TILE  " >> $TEMP_FILE ; 
    printf "<td align=center><font font-weight = \"bold\" size = \"3\" color=darkblue>%s</font></td>" "  99%TILE  " >> $TEMP_FILE ; 
    printf "</tr>\n" >> $TEMP_FILE ; 

    for files in `ls /NAS1/data/ORSData/$loc/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/*`
    do 

      fname=`echo $files | awk -F"/" '{print $NF}'` ;
      product=`echo $fname | awk -F"_" '{print $1}'` ;
      location=`echo $files | awk -F"/" '{print $5}'` ; 

      if [ "$location" == "NSE" ] ; then 

        if [ `grep -w "$product" /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt | wc -l` -gt 0 ] ; then 
          product=`grep -w "$product" /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt | awk '{print $2}'` ; 
        fi 

      fi 
      
      $ORS_BIN_EXEC ZN_0 $YYYYMMDD SUMMARY $files > "/tmp/rtts_tmp" 2>&1
      SUMMARY_STRING=`cat "/tmp/rtts_tmp"`
      rm "/tmp/rtts_tmp"

      printf "<tr><td align=center>%s</td>" $product >> $TEMP_FILE ; 
      printf "<td align=center>%d</td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $7}'` >> $TEMP_FILE ;
      printf "<td align=center>%d</td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $9}' | awk -F"." '{print $1}'` >> $TEMP_FILE ;
      printf "<td align=center><b><font font-weight=bold color=#424242>%d</font></b></td>" $benchmark_median >> $TEMP_FILE ; 


      if [ `echo $SUMMARY_STRING | grep "Shortcode" | wc -l` -le 0 ] ; then 
        continue ; 
      fi 

      if [ `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $17}'` -gt $compare_benchmark_median ] ; then  
        printf "<td align=center bgcolor=#01DFD7><b><font color=darkred>%d</font></b></td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $17}'` >> $TEMP_FILE ;
      else 
        printf "<td align=center>%d</td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $17}'` >> $TEMP_FILE ;
      fi 

      printf "<td align=center>%d</td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $21}'` >> $TEMP_FILE ;
      printf "<td align=center><b><font font-weight=bold color=#424242>%d</font></b></td>" $benchmark_high >> $TEMP_FILE ; 


      if [ `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $23}'` -gt $compare_benchmark_high ] ; then 
        printf "<td align=center bgcolor=#01DFD7><b><font color=darkred>%d</font></b></td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $23}'` >> $TEMP_FILE ;
      else 
        printf "<td align=center>%d</td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $23}'` >> $TEMP_FILE ;
      fi

      printf "<td align=center>%d</td>" `echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $25}'` >> $TEMP_FILE ;
      printf "</tr>\n" >> $TEMP_FILE ;

      median_latency=$((median_latency+`echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $17}'`)) ; 
      high_latency=$((high_latency+`echo $SUMMARY_STRING | tr ':' ' ' | awk '{print $23}'`)) ; 
      ((count++)) ; 

      echo $SUMMARY_STRING | sed "s/Shortcode :ZN_0/Symbol :$product/g" >> $DATFILE ;

    done 

    printf "</table><br><br>" >> $TEMP_FILE ; 

    TEMP_REF_FILE=/tmp/temp_refrtt.tmp ;  
    >$TEMP_REF_FILE ;  

    grep -v "DATE:$YYYYMMDD LOCATION:$loc" $REFDB > $TEMP_REF_FILE 2>/dev/null 
    echo "DATE:$YYYYMMDD LOCATION:$loc AVG_MEDIAN:`echo $median_latency" "$count | awk '{printf "%d", ($1/$2)}'` AVG_95:`echo $high_latency" "$count | awk '{printf "%d", ($1/$2)}'`" >> $TEMP_REF_FILE ; 
    cat $TEMP_REF_FILE | sort -t: -nk 2 > $REFDB ; 

    rm -rf $TEMP_REF_FILE ; 

  done 

  printf "</body></html>" >> $TEMP_FILE ; 

  cat $TEMP_FILE | /usr/sbin/sendmail -r "nseall@tworoads.co.in" "ravi.parikh@tworoads.co.in" "nseall@tworoads.co.in"

  rm -rf $TEMP_FILE ; 

}

compute_and_send_ind_rtts $* 
