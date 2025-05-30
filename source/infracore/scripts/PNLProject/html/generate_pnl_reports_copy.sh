#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}


#PNL Per date reports
PerDayPNLReports () {
  PerDayPNL_FILE=$path"html/"$server"/"$server"."$day".index.html"
  > $PerDayPNL_FILE
  #echo -e "For:: $PerDayPNL_FILE\n"
  cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/PNL REPORTS [ '$dttime' ] /g' >> $PerDayPNL_FILE ;
  echo "<th>STRATEGY_ID</th><th>PNL_COUNT</th></tr></thead><tbody>">> $PerDayPNL_FILE; 
  pnl_data_file=$path"PNLCount1/"$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd
  [ -f $pnl_data_file ] || continue
  #echo -e "per day:: $yyyy$mm$dd, $path, $server\n"
  while read line
    do
	 stratId=`echo $line | cut -d' ' -f 1`
	 pnlCount=`echo $line | cut -d' ' -f 2`
	 numopen=`echo $line | cut -d' ' -f 3`
	 #echo -e "	stratId, pnlCount, numopen:: $stratId, $pnlCount, $numopen\n"
	 echo "<tr><td>$stratId</td><td>$pnlCount</td></tr>" >> $PerDayPNL_FILE
    done < $pnl_data_file
  echo "</tbody></table></body></html>" >> $PerDayPNL_FILE
}

#Each Month PNL reports
EachMonthPNLReports () {
  server=$1 ;
  EachMonthPNL_backup_FILE=$path"html/"$server"/"$server".eachMonthPNL.backup" ;  
  PerMonthPNL_FILE=$path"html/"$server"/"$server".eachMonthPNL.index.html"
  > $PerMonthPNL_FILE 
  > $EachMonthPNL_backup_FILE
  cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/PNL REPORTS Per Month [ '$server' ] /g' >> $PerMonthPNL_FILE ;
  echo "<th>Date</th><th>TotalPNL</th></tr></thead><tbody>" >> $PerMonthPNL_FILE
  
  cd $path"PNLCount1/"$server ;
  #echo -e "Server,PWD:: $server, $PWD\n "
  for yyyy in `ls -I '*.*'`;
    do
	cd $path"PNLCount1/"$server"/"$yyyy
	#echo -e "for $path"PNLCount1/"$server\n"
        for mm in `ls -I '*.*'`;
          do
                cd $path"PNLCount1/"$server"/"$yyyy"/"$mm
             	#echo -e "       yearmm, pwd:: $yyyy $mm, $PWD\n"
		for monthlyPNL in `ls | grep 'TotalMonthlyStratPNL.*'`;
	  	  do
		    totalPnl=`awk '{t+=$2} END {print t}' $monthlyPNL`
		    totalNewOpen=`awk '{o+=$3} END {print o}' $monthlyPNL`
		    #echo -e "	date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen\n"
		    hrefRef=$path"/html/"$server"/"$yyyy"/"$mm"/"$yyyy$mm".index.html" ;
		    echo "<tr><td>$yyyy$mm</td><td><a href=$hrefRef >$totalPnl</a></td></tr>" >>$PerMonthPNL_FILE ;
		  done
	  done
    done
    echo "</tbody></table></body></html>" >>$PerMonthPNL_FILE ;
}

#specific month PNL Data
SpecificMonthPNLReports () {
    server=$1 ;
    cd $path"PNLCount1/"$server
    for yyyy in `ls -I '*.*'`;
    do
      cd $path"PNLCount1/"$server"/"$yyyy
      #echo -e "for PWD:: $PWD, $server\n"
      for mm in `ls -I '*.*'`;
      do
        cd $path"PNLCount1/"$server"/"$yyyy"/"$mm
#        echo -e "		month $mm\n"
	#cd $mm
#	echo -e "       yearmm, pwd:: $yyyy$mm, $PWD\n"
#        for file in `ls | grep 'TotalMonthlyStratPNL.*'`;
#        do
#	  echo -e "file:: $file\n"
	  file="TotalMonthlyStratPNL."$yyyy$mm ;
          SpecificMonthPNL_FILE=$path"html/"$server"/"$yyyy"/"$mm"/"$yyyy$mm".index.html" ;
	  mkdir -p $path"/html/"$server"/"$yyyy"/"$mm ;
	  >$SpecificMonthPNL_FILE 
          cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS [ '$yyyy$mm' ] /g' > $SpecificMonthPNL_FILE ;
          echo "<th>STRATEGY_ID></th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificMonthPNL_FILE ;
	  while read line
          do
            stratid=`echo $line | cut -d' ' -f 1`
            pnl=`echo $line | cut -d' ' -f 2`
	    numOpen=`echo $line | cut -d' ' -f 3`
	    hrefRef=$path"html/"$server"/"$yyyy"/"$mm"/"$yyyy$mm"."$stratid".index.html" ;
	    echo "<tr><td>$stratid</td><td><a href=$hrefRef>$pnl</a></td></tr>" >> $SpecificMonthPNL_FILE ;
	  done < $file
	  echo "</tbody></table></body></html>" >> $SpecificMonthPNL_FILE ;
        done
      done
 #   done 
  
}

#Specific Strategy Monthly PNL
SpecificStrategyMonthlyPNLReports () {
    server=$1;
    cd $path"PNLCount1/"$server
    for yyyy in `ls -I '*.*'` ;
    do
      cd $path"PNLCount1/"$server"/"$yyyy
      for mm in `ls -I '*.*'` ;
      do
        cd $path"PNLCount1/"$server"/"$yyyy"/"$mm
	#echo -e "month, pwd:: $mm, $PWD\n"
        for file in `ls | grep 'monthlyStratPNL.*'` ;
	do
	  stratid=`echo $file | cut -d'.' -f 2`
	  echo -e "	stratID::$file,  $stratid, $PWD\n"
	  SpecificStrategyMonthPNL_FILE=$path"html/"$server"/"$yyyy"/"$mm"/"$yyyy$mm"."$stratid".index.html" ;
#	  mkdir -p $path"html/"$server"/"$yyyy"/"$mm ;
	  >$SpecificStrategyMonthPNL_FILE
          cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS For Strategy_ID '$stratid' [ '$yyyy$mm' ] /g' > $SpecificStrategyMonthPNL_FILE ;
          echo "<th>Date></th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificStrategyMonthPNL_FILE ;
	  while read line
	  do
	    date=`echo $line | cut -d' ' -f 1`
	    pnl=`echo $line | cut -d' ' -f 2`
	    echo "<tr><td>$date</td><td>$pnl</td></tr>" >> $SpecificStrategyMonthPNL_FILE ;
	  done < $file
	  echo "</tbody></table></body></html>" >> $SpecificStrategyMonthPNL_FILE ;
	done
      done
    done
}

init () {

  [ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < DATE >" ;
  
  yyyymmdd=$1; 
  
  if [ "$1" == "TODAY" ] ; then 
    yyyymmdd=`date +"%Y%m%d"` ;
  fi  

  path="/home/hardik/PNLProject/";
  #rm -rf $REPORTING_DIR;
  #mkdir -p $REPORTING_DIR ;

  REPORTING_FILE=$path"html/index.html" ;
  >$REPORTING_FILE ;

  dttime=`date +"%Y%m%d.%H%M"`;

  #HEADER
  cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/PNL REPORTS [ '$dttime' ] /g' > $REPORTING_FILE ; 

  echo "<th>DATE</th>" >> $REPORTING_FILE ;

  servers="IND11 IND12 IND13" ;

  for server in $servers; 
  do
    echo "<th><a href=$path"html/"$server"/"$server".eachMonthPNL.index.html" style="color:red">$server</a></th>" >> $REPORTING_FILE ; 
  done 

  echo "</tr></thead><tbody>" >> $REPORTING_FILE ; 
  
  Mon=`date --date="$yyyymmdd -4 day" +\%Y\%m\%d`
  Tue=`date --date="$yyyymmdd -3 day" +\%Y\%m\%d`
  Wed=`date --date="$yyyymmdd -2 day" +\%Y\%m\%d`
  Thurs=`date --date="$yyyymmdd -1 day" +\%Y\%m\%d`
  Fri=$yyyymmdd

  for day in $Mon $Tue $Wed $Thurs $Fri 
  do
    echo "<tr><td>$day</td>" >>$REPORTING_FILE;
    for server in $servers;
    do
      
      yyyy=${day:0:4};
      mm=${day:4:2};  
      dd=${day:6:2};
      #echo -e "for server(yyyymmdd):: $server($yyyy$mm$dd)\n"
      [ -f $path"PNLCount1/"$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd ] || continue	
      mkdir -p $path"html/"$server ;      
      PerDayPNLReports ; 
      totalpnl=`awk '{t+=$2} END {print t}' $path"PNLCount1/"$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd`
      #echo -e "day, server, pnl:: $day, $server, $totalpnl\n"
      echo "<td><a href=$path"html/"$server"/"$server"."$day".index.html" style="color:blue">$totalpnl</a></td>" >> $REPORTING_FILE ;
    done
    echo "</tr>" >> $REPORTING_FILE;
  done 
  echo "</tbody></table></body></html>" >> $REPORTING_FILE;

  for server in $servers; 
  do
    echo -e "for server:: $server\n" ;
    EachMonthPNLReports $server ;
    SpecificMonthPNLReports $server ;
    SpecificStrategyMonthlyPNLReports $server ;
  done 

 
#  SpecificStrategyMonthlyPNLReports ;'

}

init $*
