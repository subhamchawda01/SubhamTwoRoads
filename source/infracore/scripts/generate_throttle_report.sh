#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

#PNL Per date reports
PerDayThrottleReports () { 
  PerDayThrottle_FILE=$destpath"daily_throttle_file/"$day".index.html"
  > $PerDayThrottle_FILE ;
  cat $path"daily_throttle_report_header.txt" | sed 's/Throttle REPORTS/Throttle REPORTS [ '$day' ] /g' >> $PerDayThrottle_FILE ;
  echo "<th>Server</th><th>Mean</th><th>Median</th><th>Total</th><th>MaxTHrottleMin</th><th>OF_0</th><th>OF_1</th><th>CXLRe</th><th>CxLReRejc</th><th>Rejc</th><th>Start Time</th><th>End Time</th><th>Max Time</th></tr></thead><tbody>" >> $PerDayThrottle_FILE
  throttle_data_file=$file
  [ -f $throttle_data_file ] || continue
  while read line
    do
         serverID=`echo $line | cut -d' ' -f 2`
         mean=`echo $line | cut -d' ' -f 4`
         median=`echo $line | cut -d' ' -f 6`
	 total=`echo $line | cut -d' ' -f 8`
         maxthrottle=`echo $line | cut -d' ' -f 10`
         op0=`echo $line | cut -d' ' -f 12` 
	 op1=`echo $line | cut -d' ' -f 14`
         cxre=`echo $line | cut -d' ' -f 16`
         cxrerejc=`echo $line | cut -d' ' -f 18`
	 rejc=`echo $line | cut -d' ' -f 20`
	 startTime=`echo $line | cut -d' ' -f 22`
  	 endTime=`echo $line | cut -d' ' -f 24`
	 maxTime=`echo $line | cut -d' ' -f 37`
        echo "<tr><td><b>${ServerMap[$serverID]}</b></td><td>$mean</td><td>$median</td><td>$total</td><td>$maxthrottle</td><td>$op0</td><td>$op1</td><td>$cxre</td><td>$cxrerejc</td><td>$rejc</td><td>$startTime</td><td>$endTime</td><td>$maxTime</td></tr>" >> $PerDayThrottle_FILE
	echo "<tr><td><b>$day</b></td><td>$mean</td><td>$median</td><td>$total</td><td>$maxthrottle</td><td>$op0</td><td>$op1</td><td>$cxre</td><td>$cxrerejc</td><td>$rejc</td><td>$startTime</td><td>$endTime</td><td>$maxTime</td></tr>" >> "${destpath}dailydate_throttle_file/${ServerMap[$serverID]}.index.html"
	#echo "<td>$mean</td>" >> $REPORTING_FILE
	#echo $serverID
	meanMap[${ServerMap[$serverID]}]=$mean
	#echo "${meanMap[${ServerMap[$serverID]}]}"
	done < $throttle_data_file


  cat $path"daily_throttle_report_footer.txt" >> $PerDayThrottle_FILE
}

init () {

  path="/home/pengine/prod/live_scripts/ThrottleProject/script/";

  destpath="/var/www/html/ThrottleProject/"
  readpath="/home/dvcinfra/important/ThrottleProject/throttle_generated"
  REPORTING_FILE=$destpath"index.html" ;
  >$REPORTING_FILE ;

  mkdir -p $destpath ;
  mkdir -p "${destpath}dailydate_throttle_file" ;
  mkdir -p $destpath"daily_throttle_file" ;
  cat $path"throttle_report_header.txt" > $REPORTING_FILE ;


  servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20 IND303 IND304 IND23 IND24" ;
  declare -A ServerMap=([303]=IND303 [304]=IND304 [323]=IND323 [305]=IND15 [308]=IND16 [307]=IND17 [306]=IND18 [314]=IND14 [320]=IND20 [322]=IND22 [340]=IND19 [330]=IND21 [312]=IND23 [344]=IND24)
  declare -A meanMap;
  echo "<th>Date</th>" >> $REPORTING_FILE;
  for server in $servers;  do
    echo "<th><a href =$despath"dailydate_throttle_file/"$server".index.html" style="color:blue">$server</a></th>" >> $REPORTING_FILE ;
    cat $path"daily_throttle_report_header.txt" | sed 's/Throttle REPORTS/Server Throttle REPORTS [ '$server' ] /g' > "${destpath}dailydate_throttle_file/${server}.index.html"
    echo "<th>Date</th><th>Mean</th><th>Median</th><th>Total</th><th>MaxTHrottleMin</th><th>OF_0</th><th>OF_1</th><th>CXLRe</th><th>CxLReRejc</th><th>Rejc</th><th>Start Time</th><th>End Time</th><th>Max Time</th></tr></thead><tbody>" >> "${destpath}dailydate_throttle_file/${server}.index.html"
  done
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
 
  for file in $readpath/*
  do
    day=$(basename $file);
    #echo $file" " $day
    echo "<tr><td><a href =$despath"daily_throttle_file/"$day".index.html" style="color:blue">$day</a></td>" >>$REPORTING_FILE;
      mkdir -p $destpath"/daily_throttle_file" ;
      PerDayThrottleReports ; 
      	for server in $servers; do
     		 echo "<td>${meanMap[$server]}</td>" >> $REPORTING_FILE;
		 #echo "${meanMap[$server]}"
		 meanMap[$server]=''
	done
    echo "</tr>" >> $REPORTING_FILE;
  done
  for server in $servers;do
	cat $path"daily_throttle_report_footer.txt" >> "${destpath}dailydate_throttle_file/${server}.index.html"
  done
  cat $path"throttle_report_footer.txt" >> $REPORTING_FILE;
}

init $*

