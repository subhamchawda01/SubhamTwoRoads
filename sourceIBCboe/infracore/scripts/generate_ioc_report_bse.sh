#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

#PNL Per date reports
PerDayIOCReports () { 
  PerDayIOC_FILE=$destpath"daily_ioc_file/"$day".index.html"
  > $PerDayIOC_FILE ;
  cat $path"daily_throttle_report_header.txt" | sed 's/Throttle REPORTS/IOC REPORTS [ '$day' ] /g' >> $PerDayIOC_FILE ;

        echo "<div class='row header' style='text-align:center;color:green'>" >>$PerDayIOC_FILE
	echo "<th>Server</th><th>Seqd</th><th>Exec</th><th>Cxld</th><th>RatioExec %</th><th>RatioCxld %</th></tr></thead><tbody>" >> $PerDayIOC_FILE
  ioc_data_file=$file
  [ -f $ioc_data_file ] || continue
  while read line
    do
	 serverID=`echo $line | cut -d' ' -f 2`
         seqd=`echo $line | cut -d' ' -f 26`
	 exe=`echo $line | cut -d' ' -f 28`
	 cxld=`echo $line | cut -d' ' -f 30`
	 ratioexe=`echo $line | cut -d' ' -f 32`
	 ratiocxld=`echo $line | cut -d' ' -f 34`
	 tot=`expr $seqd + $exe + $cxld`
         echo "<tr><td><b>${ServerMap[$serverID]}</b></td><td>$seqd</td><td>$exe</td><td>$cxld</td><td>$ratioexe</td><td>$ratiocxld</td></tr>" >> $PerDayIOC_FILE
	 echo "<tr><td><b>$day</b></td><td>$seqd</td><td>$exe</td><td>$cxld</td><td>$ratioexe</td><td>$ratiocxld</td></tr>" >> "${destpath}dailydate_ioc_file/${ServerMap[$serverID]}.index.html"
         meanMap[${ServerMap[$serverID]}]=$ratioexe
	 done < $ioc_data_file

  cat $path"daily_throttle_report_footer.txt" >> $PerDayIOC_FILE
}

init () {

  path="/home/pengine/prod/live_scripts/ThrottleProject/script/";

  destpath="/var/www/html/IOC/"
  readpath="/home/dvcinfra/important/ThrottleProject/throttle_generated"
  REPORTING_FILE=$destpath"index.html" ;
  >$REPORTING_FILE ;

  mkdir -p $destpath ;
  mkdir -p "${destpath}daily_ioc_file" ;
  mkdir -p "${destpath}dailydate_ioc_file" ;
  cat $path"throttle_report_header.txt"| sed 's/Throttle REPORTS/IOC REPORTS/g' > $REPORTING_FILE ;


  servers="INDB12" ;
  declare -A ServerMap=([400]=INDB12)
  declare -A meanMap;
  echo "<th>Date</th>" >> $REPORTING_FILE;
  for server in $servers;  do
    echo "<th><a href =$despath"dailydate_ioc_file/"$server".index.html" style="color:blue">$server</a></th>" >> $REPORTING_FILE ;
    cat $path"daily_throttle_report_header.txt" | sed 's/Throttle REPORTS/Server IOC REPORTS [ '$server' ] /g' > "${destpath}dailydate_ioc_file/${server}.index.html"
    echo "<th>Date</th><th>Seqd</th><th>Exec</th><th>Cxld</th><th>RatioExec %</th><th>RatioCxld %</th></tr></thead><tbody>" >> "${destpath}dailydate_ioc_file/${server}.index.html"
  done
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
 
  for file in $readpath/*
  do
    day=$(basename $file);
    #echo $file" " $day
    echo "<tr><td><a href =$despath"daily_ioc_file/"$day".index.html" style="color:blue">$day</a></td>" >>$REPORTING_FILE;
      mkdir -p $destpath"/daily_ioc_file" ;
      PerDayIOCReports ; 
      	for server in $servers; do
     		 echo "<td>${meanMap[$server]}</td>" >> $REPORTING_FILE;
		 #echo "${meanMap[$server]}"
		 meanMap[$server]=''
	done
    echo "</tr>" >> $REPORTING_FILE;
  done
  for server in $servers;do
        cat $path"daily_throttle_report_footer.txt" >> "${destpath}dailydate_ioc_file/${server}.index.html"
  done
  cat $path"throttle_report_footer.txt" >> $REPORTING_FILE;
}

init $*

