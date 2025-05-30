#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

#Each Month PNL reports
EachMonthPNLReports () {
  #server=$1 ;
  mkdir -p $destpath$server 
  EachMonthPNL_backup_FILE=$destpath$server"/"$server".eachMonthPNL.backup" ;  
  > $EachMonthPNL_backup_FILE
  PerMonthPNL_FILE=$destpath$server"/"$server".eachMonthPNL.index.html"
  > $PerMonthPNL_FILE
  cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/PNL REPORTS Per Month [ '$server' ] /g' >> $PerMonthPNL_FILE ;
  echo "<th>Date</th><th>TotalPNL</th></tr></thead><tbody>" >> $PerMonthPNL_FILE


  cd $path"PNLCount1/"$server ;
#  echo -e "Server,PWD:: $server, $PWD\n "
  for yyyy in `ls -I '*.*'`;
    do
	cd $path"PNLCount1/"$server"/"$yyyy
	#echo -e "for $path"PNLCount1/"$server\n"
        for mm in `ls -I '*.*'`;
          do
                cd $path"PNLCount1/"$server"/"$yyyy"/"$mm
             	#echo -e "       yearmm, pwd:: $yyyy $mm, $PWD\n"
		monthlyPNL="TotalMonthlyStratPNL."$yyyy$mm;
	        totalPnl=`awk '{t+=$2} END {print t}' $monthlyPNL`
	        totalNewOpen=`awk '{o+=$3} END {print o}' $monthlyPNL`
		    #echo -e "	date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen\n"
	         hrefRef=$yyyy"/"$mm"/"$yyyy$mm".index.html" ;
         	 echo "<tr><td>$yyyy$mm</td><td><a href=$hrefRef>$totalPnl</a></td></tr>" >>$EachMonthPNL_backup_FILE ;
	  done
 #         echo -e "for $server\n"; 
#	  cat $EachMonthPNL_backup_FILE
#	  echo -e "\nend\n\n"
    done
    cat $EachMonthPNL_backup_FILE >>$PerMonthPNL_FILE ;
#    echo "</tbody></table></body></html>" 
    cat $path"html/pnl_report_footer.txt" >> $PerMonthPNL_FILE ;
}


#specific month PNL Data
SpecificMonthPNLReports () {
   # server=$1 ;
    cd $path"PNLCount1/"$server
    for yyyy in `ls -I '*.*'`;
    do
      cd $path"PNLCount1/"$server"/"$yyyy
      #echo -e "for PWD:: $PWD, $server\n"
      for mm in `ls -I '*.*'`;
      do
        cd $path"PNLCount1/"$server"/"$yyyy"/"$mm
#        echo -e "		month $mm\n"
#	echo -e "       yearmm, pwd:: $yyyy$mm, $PWD\n"
#	  echo -e "file:: $file\n"
	  file="TotalMonthlyStratPNL."$yyyy$mm ;
          SpecificMonthPNL_FILE=$destpath$server"/"$yyyy"/"$mm"/"$yyyy$mm".index.html" ;
	  mkdir -p $destpath$server"/"$yyyy"/"$mm ;
	  >$SpecificMonthPNL_FILE 
          cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS [ '$yyyy$mm' ] /g' > $SpecificMonthPNL_FILE ;
          echo "<th>STRATEGY_ID</th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificMonthPNL_FILE ;
	  while read line
          do
            stratid=`echo $line | cut -d' ' -f 1`
            pnl=`echo $line | cut -d' ' -f 2`
	    numOpen=`echo $line | cut -d' ' -f 3`
	    hrefRef=$yyyy$mm"."$stratid".index.html" ;
	    echo "<tr><td>$stratid</td><td><a href=$hrefRef>$pnl</a></td></tr>" >> $SpecificMonthPNL_FILE ;
	  done < $file
	 # echo "</tbody></table></body></html>" >> 
          cat $path"html/pnl_report_footer.txt" >> $SpecificMonthPNL_FILE ;
        done
      done
}

#Specific Strategy Monthly PNL
SpecificStrategyMonthlyPNLReports () {
    #server=$1;
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
#	  echo -e "	stratID::$file,  $stratid, $PWD\n"
	  SpecificStrategyMonthPNL_FILE=$destpath$server"/"$yyyy"/"$mm"/"$yyyy$mm"."$stratid".index.html" ;
	  mkdir -p $destpath$server"/"$yyyy"/"$mm ;
	  >$SpecificStrategyMonthPNL_FILE
          cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS For Strategy_ID '$stratid' [ '$yyyy$mm' ] /g' > $SpecificStrategyMonthPNL_FILE ;
          echo "<th>Date</th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificStrategyMonthPNL_FILE ;
	  while read line
	  do
	    date=`echo $line | cut -d' ' -f 1`
	    pnl=`echo $line | cut -d' ' -f 2`
	    echo "<tr><td>$date</td><td>$pnl</td></tr>" >> $SpecificStrategyMonthPNL_FILE ;
	  done < $file
#	  echo "</tbody></table></body></html>" >>
          cat $path"html/pnl_report_footer.txt" >> $SpecificStrategyMonthPNL_FILE ;
	done
      done
    done
}

init () {
  servers="IND15 IND16 IND17 IND18 IND19 IND20" ;
  path="/home/hardik/PNLProject/";
  destpath="/NAS1/data/PNLReportsIND/www/PNLReportsIND/";
  for server in $servers; 
  do
 #   echo -e "for server:: $server\n" ;
    EachMonthPNLReports $server ;
    SpecificMonthPNLReports $server ;
    SpecificStrategyMonthlyPNLReports $server ;
  done 
}

init $*
