#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

#PNL Per date reports
PerDayPNLReports () {
  PerDayPNL_FILE=$destpath$server"/"$server"."$day".index.html"
  > $PerDayPNL_FILE
  echo -e "For:: $PerDayPNL_FILE"
  cat $path"html/pnl_report_header.txt" | sed "s/PNL REPORTS/PNL REPORTS [ ${day} ] /g" >> $PerDayPNL_FILE ;
  echo "<th>STRATEGY_ID</th><th>PNL_COUNT</th></tr></thead><tbody>">> $PerDayPNL_FILE; 
  pnl_data_file=$path"PNLCount1/"$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd
  [ -f $pnl_data_file ] || continue
  #echo -e "per day:: $yyyy$mm$dd, $path, $server\n"
  while read line
    do
	 stratId=`echo $line | cut -d' ' -f 1`
	 pnlCount=`echo $line | cut -d' ' -f 2`
	 numopen=`echo $line | cut -d' ' -f 3`
	 echo -e "	stratId, pnlCount, numopen:: $stratId, $pnlCount, $numopen $server\n"
	 echo "<tr><td><a href=$yyyy"/"$mm"/"$dd"/"$stratId".top_gainers_losers.html" style="color:blue">$stratId</a></td><td>$pnlCount</td></tr>" >> $PerDayPNL_FILE
    done < $pnl_data_file
 # echo "</tbody></table></body></html>" >>
  cat $path"html/pnl_report_footer.txt" >> $PerDayPNL_FILE
}

init () {
  [ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < DATE >" ;
  declare -a lastFiveDates=();
  yyyymmdd=$1; 
  if [ "$1" == "YESTERDAY" ] ; then 
    yyyymmdd=`date -d "1 day ago" +"%Y%m%d"` ;
  fi  
  path="/home/hardik/PNLProject/";
  destpath="/NAS1/data/PNLReportsIND/www/PNLReportsIND/";
  REPORTING_FILE=$destpath"/index.html" ;
  >$REPORTING_FILE ;
  dttime=`date +"%Y%m%d.%H%M"`;
  #HEADER
  cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/PNL REPORTS /g' > $REPORTING_FILE ; 
  servers="IND14 IND15 IND19 IND20 IND16 IND17 IND18" ;
  echo "<th>Date</th>" >> $REPORTING_FILE;
  for server in $servers; 
  do
    echo "<th><a href=$server"/"$server".eachMonthPNL.index.html" style="color:red">$server</a></th>" >> $REPORTING_FILE ; 
  done 
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ; 
  i=-1;
  for (( c=1; c<6 ; c++ ))
  do
   i=$((i + 1))
   temp=`date --date="$yyyymmdd -$i day" +%a`
   tempdate=`date --date="$yyyymmdd -$i day" +%Y%m%d`  
  #echo -e "for c= $c\n"
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $tempdate T`
  #echo -e "holiday-> $is_holiday\n"
  while [ "$temp" = "Sun" ] || [ "$is_holiday" = "1" ]
   do
     #echo -e "\ti= $i\n"
     i=$((i + 1))
     temp=`date --date="$yyyymmdd -$i day" +%a`
     tempdate=`date --date="$yyyymmdd -$i day" +%Y%m%d`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $tempdate T`
   done
   temp=`date --date="$yyyymmdd -$i day" +\%Y\%m\%d`
   #echo -e "c, i, temp=> $c, $i, $temp\n"
   lastFiveDates+=("$temp")
 done
  for day in ${lastFiveDates[@]} 
  do
    echo "<tr><td>$day</td>" >>$REPORTING_FILE;
    for server in $servers;
    do 
      yyyy=${day:0:4};
      mm=${day:4:2};  
      dd=${day:6:2};
      #echo -e "for server(yyyymmdd):: $server($yyyy$mm$dd)\n"
      if [ -f $path"PNLCount1/"$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd ]; then
      #echo -e $path"PNLCount1/"$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd" exist\n" 
      mkdir -p $destpath$server ;      
      PerDayPNLReports ; 
      totalpnl=`awk '{t+=$2} END {print t}' $path"PNLCount1/"$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd`
      #echo -e "day, server, pnl:: $day, $server, $totalpnl\n"
      echo "<td><a href="$server"/"$server"."$day".index.html" style='color:blue'">$totalpnl</a></td>" >> $REPORTING_FILE ;
      else
          echo "<td> </td>" >> $REPORTING_FILE ;
      fi
    done
    echo "</tr>" >> $REPORTING_FILE;
  done 
  cat $path"html/pnl_report_footer.txt" >> $REPORTING_FILE;
}

init $*
