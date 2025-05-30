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
  # yearly data generated
  cat $path"html/pnl_report_header2.txt" | sed 's/PNL REPORTS/PNL REPORTS Per YEAR [ '$server' ] /g' >> $PerMonthPNL_FILE ;
  echo "<table class="table table-striped" style='border: 1px solid grey;margin-bottom: 0px;'><thead><tr>" >> $PerMonthPNL_FILE
  echo "<th>Year</th><th>Graph</th><th>TotalPNL</th></tr></thead><tbody>" >> $PerMonthPNL_FILE
  cd $path"PNLCount1/"$server ;

  for yyyy in `ls -I '*.*'`;
  do
        Y_PNL=$yyyy"/TotalYearlyStratPNL."$yyyy;
        totalPnl=`awk '{t+=$1} END {print t}' $Y_PNL`
        hrefImg=$yyyy"/"$server"."$yyyy".jpeg"
        mkdir -p $destpath$server'/'$yyyy
        GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'$destpath$server'/'$yyyy'/'$server'.'$yyyy'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$path'PNLCount1/'$server'/'$yyyy'/YearlyStratPNL.'$yyyy'" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'
                echo $GNUPLOT | gnuplot ;
        echo "<tr><td><a href=$yyyy"/top_gainers_losers.html" style="color:blue">$yyyy</a></td><td><a href=$hrefImg><img border=0 src='../Graph-512.png' width=40 height=25></a></td><td><a href=$yyyy"/"$yyyy".index.html">$totalPnl</a></td></tr>" >>$PerMonthPNL_FILE ;
  done
  echo "</tbody></table>" >> $PerMonthPNL_FILE
  echo "<div class='row header' style='text-align:center;color:green'><h3>PNL REPORTS PER MONTH [ $server ]</h3></div>" >> $PerMonthPNL_FILE
  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $PerMonthPNL_FILE
  echo "<th>Date</th><th>Graph</th><th>TotalPNL</th></tr></thead><tbody>" >> $PerMonthPNL_FILE

  cd $path"PNLCount1/"$server ;
#  echo -e "Server,PWD:: $server, $PWD\n "
  for yyyy in `ls -I '*.*'`;
    do
	cd $path"PNLCount1/"$server"/"$yyyy
	#echo -e "for $path"PNLCount1/"$server\n"
        for mm in `ls -I '*.*'`;
          do
               hrefImg=$yyyy"/"$mm"/"$server"."$yyyy$mm".jpeg"
	       mkdir -p $destpath$server'/'$yyyy'/'$mm
               GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy$mm'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'$destpath$server'/'$yyyy'/'$mm'/'$server'.'$yyyy$mm'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$path'PNLCount1/'$server'/'$yyyy'/'$mm'/TotalMonthlyStratPNL.'$yyyy$mm'" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'
                echo $GNUPLOT | gnuplot ;

                cd $path"PNLCount1/"$server"/"$yyyy"/"$mm
             	#echo -e "       yearmm, pwd:: $yyyy $mm, $PWD\n"
		monthlyPNL="TotalMonthlyStratPNL."$yyyy$mm;
	        totalPnl=`awk '{t+=$2} END {print t}' $monthlyPNL`
	        totalNewOpen=`awk '{o+=$3} END {print o}' $monthlyPNL`
		    #echo -e "	date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen\n"
	         hrefRef=$yyyy"/"$mm"/"$yyyy$mm".index.html" ;
         	 echo "<tr><td><a href=$yyyy"/"$mm"/""top_gainers_losers.html" style="color:blue">$yyyy$mm</a></td><td><a href=$hrefImg><img border=0 src='../Graph-512.png' width=40 height=25></a></td><td><a href=$hrefRef>$totalPnl</a></td></tr>" >>$EachMonthPNL_backup_FILE ;
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
          echo "<th>STRATEGY_ID</th><th>Graph</th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificMonthPNL_FILE ;
	  while read line
          do
            stratid=`echo $line | cut -d' ' -f 1`
            pnl=`echo $line | cut -d' ' -f 2`
	    numOpen=`echo $line | cut -d' ' -f 3`
	    hrefRef=$yyyy$mm"."$stratid".index.html" ;
            hrefImg=$server"."$stratid"."$yyyy$mm".jpeg"
            GNUPLOT="set terminal jpeg size 1366,768; set xtics rotate; set title '$stratid->$yyyy$mm'; set xlabel 'Date'; set ylabel 'PNL count'; set output '$destpath$server/$yyyy/$mm/$server.$stratid.$yyyy$mm.jpeg'; set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 pi -1 ps 1.5; set pointintervalbox 3; set grid; plot \"<awk '{i+=\$2} {print \$1,i}'  $path'PNLCount1/'$server/$yyyy/$mm/monthlyStratPNL.$stratid\" u (column(0)):2:xtic(1) with linespoints ls 1 title ''"            
            echo $GNUPLOT | gnuplot ;
            echo "<tr><td>$stratid</td><td><a href=$hrefImg><img border=0 src='../../../Graph-512.png' width=40 height=25></a></td><td><a href=$hrefRef>$pnl</a></td></tr>" >> $SpecificMonthPNL_FILE ;
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
            dd=${date:6:2};
	    echo "<tr><td><a href=$dd"/"$stratid".top_gainers_losers.html" style="color:blue">$date</a></td><td>$pnl</td></tr>" >> $SpecificStrategyMonthPNL_FILE ;
	  done < $file
#	  echo "</tbody></table></body></html>" >>
          cat $path"html/pnl_report_footer.txt" >> $SpecificStrategyMonthPNL_FILE ;
	done
      done
    done
}
#specific Year PNL Data
SpecificYearPNLReports () {
      #server=$1 ;
      cd $path"PNLCount1/"$server
      for yyyy in `ls -I '*.*'` ;
      do
      cd $path"PNLCount1/"$server"/"$yyyy
      echo -e "Specific Year($server:$yyyy)::\n"
      file="YearlyStratPNL."$yyyy
      SpecificYearPNL_FILE=$destpath$server"/"$yyyy"/"$yyyy".index.html" ;
      echo "Year file" $SpecificYearPNL_FILE
      mkdir -p $destpath$server"/"$yyyy ;
      >$SpecificYearPNL_FILE
      cat $path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS [ '$yyyy' ] /g' > $SpecificYearPNL_FILE ;
      echo "<th>STRATEGY_ID</th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificYearPNL_FILE ;
      while read line
      do
        stratid=`echo $line | cut -d' ' -f 1`
        pnl=`echo $line | cut -d' ' -f 2`
        numOpen=`echo $line | cut -d' ' -f 3`
        echo -e "       stratid:: $stratid\n"
        #`awk '{i+=$2} {print $1,i}' $path'PNLCount1/'$server'/'$yyyy'/'$mm'/monthlyStratPNL.'$stratid`
        echo "<tr><td>$stratid</td><td>$pnl</td></tr>" >> $SpecificYearPNL_FILE ;
      done < $file
#      echo "</tbody></table></body></html>" >> 
      cat $path"html/pnl_report_footer.txt" >> $SpecificYearPNL_FILE ;
      done
}
init () {
  servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20" ;
  path="/home/hardik/PNLProject/";
  destpath="/NAS1/data/PNLReportsIND/www/PNLReportsIND/";
  rm -rf ${destpath}IND*
  for server in $servers; 
  do
 #   echo -e "for server:: $server\n" ;
    EachMonthPNLReports $server ;
    SpecificMonthPNLReports $server ;
    SpecificStrategyMonthlyPNLReports $server ;
    SpecificYearPNLReports $server;   
  done 
}

init $*
