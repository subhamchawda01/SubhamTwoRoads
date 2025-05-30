#!/bin/bash

print_msg_and_exit (){
 echo $1;
 exit
}

servers="IND15 IND16 IND17 IND18 IND19 IND23" ;
SourceDir="/home/dvctrader/important/CASH_SIM_Result/"
Path="/home/dvctrader/SimPNLProject/";
PNLDataPath="/home/dvctrader/SimPNLProject/SimPNLCount1/";
REPORTS_DEST_DIR="/NAS1/data/SimPNLReportsIND/www/SimPNLReportsIND/";
SERVER_REPORTING_FILE=$REPORTS_DEST_DIR"index.html" ;

file_list="/home/dvctrader/SimPNLProject/file_list.txt"

declare -A ServerMap=( [IND16]=308 [IND17]=307 [IND18]=306 [IND23]=312)

declare -a day_a=(01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31)
declare -a month_a=(01 02 03 04 05 06 07 08 09 10 11 12)
declare -a year_a=(2020 2021 2022)


Create_Daily_Top_Products_File () {
        echo "Create_Daily_Top_Products_File"
        curr_top_gainer_losers_stratNo_file=$1
        curr_html_file=$2
        curr_date=$3
        echo -e "Create_Daily_Top_Products_File:: $curr_top_gainer_losers_stratNo_file $curr_html_file $curr_date \n";
        cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS For Strategy_ID '$curr_stratNo' [ '$curr_date' ] /g' > $curr_html_file ;
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th></tr></thead><tbody>">> $curr_html_file ;
        while read line
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                curr_pnl=`echo $line | cut -d' ' -f 2`
                curr_msg_count=`echo $line | cut -d' ' -f 3`
                echo "<tr><td>$curr_product</td><td>$curr_pnl</td><td>$curr_msg_count</td></tr>" >> $curr_html_file;
        done < $curr_top_gainer_losers_stratNo_file
        cat $Path"html/pnl_report_footer.txt" >> $curr_html_file;
        echo "DONE: Create_Daily_Top_Products_File , $curr_html_file"
}

Get_Curr_Top_Gainers_Losers () {
    echo "Get_Curr_Top_Gainers_Losers , file ${PNLDataPath}${server}/${yyyy}${mm}${dd}_top_gainers_losers_${stratNo}";
    mkdir -p $PNLDataPath$server
    mkdir -p $REPORTS_DEST_DIR$server
    #top 5 gainers
    #echo -e " top gainers:: \n"
    #echo $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_gainers$stratNo
    #echo -e "$products\n" | tail -6 | tac  > $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_gainers$stratNo
    #top 5 losers
    #echo -e " top losers:: \n"
    #echo -e "$products\n" | head -5 > $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_losers$stratNo
    echo "$products" > "${PNLDataPath}${server}/${yyyy}${mm}${dd}_top_gainers_losers_${stratNo}"
    Create_Daily_Top_Products_File ${PNLDataPath}${server}"/"${yyyy}${mm}${dd}"_top_gainers_losers_"${stratNo} $Daily_Top_Gainer_Loser_FILE $date
    echo "DONE Get_Curr_Top_Gainers_Losers , file ${PNLDataPath}${server}/${yyyy}${mm}${dd}_top_gainers_losers_${stratNo}"
}

Per_Day_PNLReports () {
  PerDayPNL_FILE="$REPORTS_DEST_DIR${server}/${server}_${date}_index.html"
  > $PerDayPNL_FILE
  echo -e "For:: $PerDayPNL_FILE"
  cat $Path"html/pnl_report_header.txt" | sed "s/PNL REPORTS/SIM PNL REPORTS [ ${date} ] /g" >> $PerDayPNL_FILE ;
  echo "<th>STRATEGY_ID</th><th colspan="2">PNL_COUNT</th></tr></thead><tbody>">> $PerDayPNL_FILE;
  pnl_data_file="${Path}SimPNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL"
  [ -f $pnl_data_file ] || continue
  while read line
    do
         stratId=`echo $line | cut -d' ' -f 1`
         pnlCount=`echo $line | cut -d' ' -f 2`
         numopen=`echo $line | cut -d' ' -f 3`
         echo -e "      stratId, pnlCount, numopen:: $stratId, $pnlCount, $numopen $server\n"
         livefileexist=`ssh -nq dvcinfra@10.23.5.67 [[ -f /home/dvcinfra/PNLProject/PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL ]] && echo 1 || echo 0`
         if [[ $livefileexist -eq 1 ]];then
           livestratpnl=`ssh -n dvcinfra@10.23.5.67 "grep $stratId '/home/dvcinfra/PNLProject/PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL' | awk '{print \\$2}'"`
           echo "LIVE STRATEGY PNL: $livestratpnl" ; echo ;
         else
           livestratpnl="-"
         fi
         simpnl20=$(( $pnlCount / 5 )); simpnl50=$(( $pnlCount / 2 ));
         if [[ $livestratpnl == '-' ]]; then
           color="black"
         elif [[ $pnlCount -gt 0 ]];then
               if [[ $livestratpnl -lt $(( $pnlCount + $simpnl50 )) ]] && [[ $livestratpnl -gt $(( $pnlCount - $simpnl50 )) ]]; then
                   if [[ $livestratpnl -lt $(( $pnlCount + $simpnl20 )) ]] && [[ $livestratpnl -gt $(( $pnlCount - $simpnl20 )) ]]; then
                       color="green"
                   else
                       color="Slateblue"
                   fi
               else
                   color="red"
               fi
         else
              if [[ $livestratpnl -gt $(( $pnlCount + $simpnl50 )) ]] && [[ $livestratpnl -lt $(( $pnlCount - $simpnl50 )) ]]; then
                   if [[ $livestratpnl -gt $(( $pnlCount + $simpnl20 )) ]] && [[ $livestratpnl -lt $(( $pnlCount - $simpnl20 )) ]]; then
                       color="green"
                   else
                       color="Slateblue"
                   fi
               else
                   color="red"
               fi
         fi
         echo "<tr><td><a href="${yyyy}${mm}${dd}_${stratId}_top_gainers_losers.html" style='color:$color'>$stratId</a></td><td>$pnlCount</td><td>$livestratpnl</td></tr>" >> $PerDayPNL_FILE
    done < $pnl_data_file
  cat $Path"html/pnl_report_footer.txt" >> $PerDayPNL_FILE
}


Get_Daily_Top_Gainers_Losers () {
        echo "Get Top Gainer Daily , file ${REPORTS_DEST_DIR}${server}/${yyyy}${mm}${dd}_top_gainers_losers.html"
        declare -A shortcode_pnl
        declare -A shortcode_msg_count
        declare -A shortcode_conf_count
        declare -A shortcode_cxld_count
        declare -A shortcode_cxre_count
        declare -A shortcode_OF_0_count
        declare -A shortcode_OF_1_count

        shortcode_pnl=()
        shortcode_msg_count=()
        shortcode_conf_count=()
        shortcode_cxld_count=()
        shortcode_cxre_count=()
        shortcode_OF_0_count=()
        shortcode_OF_1_count=()

        Daily_Top_Gainer_Loser="${REPORTS_DEST_DIR}${server}/${yyyy}${mm}${dd}_top_gainers_losers.html" ;
        cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS Month [ '$mm' ] year ['$yyyy' ] Day [ '$dd' ] /g' > $Daily_Top_Gainer_Loser ;
        while read line 
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                curr_pnl=`echo $line | cut -d' ' -f 2`
                curr_msg_count=`echo $line | cut -d' ' -f 3`
                #echo "$curr_product $curr_pnl $curr_msg_count"
                if [ ${shortcode_pnl[$curr_product]+abc} ] ; then
                        shortcode_pnl[$curr_product]=`echo "${shortcode_pnl[$curr_product]} + $curr_pnl" | bc`
                        shortcode_msg_count[$curr_product]=`echo "${shortcode_msg_count[$curr_product]} + $curr_msg_count" | bc`
                else
                        shortcode_pnl[$curr_product]=$curr_pnl
                        shortcode_msg_count[$curr_product]=$curr_msg_count
                fi
        done < $Daily_stratFile

        cp $Daily_stratFile $Daily_stratFile_backup
        >$Daily_stratFile
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th></thead><tbody>">> $Daily_Top_Gainer_Loser ;
        for k in "${!shortcode_pnl[@]}"
        do
                  curr_product=$k
                  curr_pnl=${shortcode_pnl["$k"]}
                  curr_msg_count=${shortcode_msg_count["$k"]}
                  echo "$curr_product $curr_pnl $curr_msg_count"
                  echo "<tr><td>$curr_product</td><td>$curr_pnl</td><td>$curr_msg_count</td></tr>" >> $Daily_Top_Gainer_Loser;
                  echo "$curr_product $curr_pnl $curr_msg_count" >> $Daily_stratFile

        done
        cat $Path"html/pnl_report_footer.txt" >> $Daily_Top_Gainer_Loser; 
        echo "DONE: Get Top Gainer Daily , file $Daily_Top_Gainer_Loser"
}

Get_Month_Top_Gainers_Losers () {
        echo "Get Top Gainer Monthly , file ${REPORTS_DEST_DIR}${server}/${yyyy}${mm}_top_gainers_losers.html"
        declare -A shortcode_pnl
        declare -A shortcode_msg_count
        declare -A shortcode_conf_count
        declare -A shortcode_cxld_count
        declare -A shortcode_cxre_count
        declare -A shortcode_OF_0_count
        declare -A shortcode_OF_1_count

        shortcode_pnl=()
        shortcode_msg_count=()
        shortcode_conf_count=()
        shortcode_cxld_count=()
        shortcode_cxre_count=()
        shortcode_OF_0_count=()
        shortcode_OF_1_count=()

        Monthly_Top_Gainer_Loser="${REPORTS_DEST_DIR}${server}/${yyyy}${mm}_top_gainers_losers.html" ;
        cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS Month [ '$mm' ] year ['$yyyy' ] /g' > $Monthly_Top_Gainer_Loser ;
        while read line 
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                curr_pnl=`echo $line | cut -d' ' -f 2`
                curr_msg_count=`echo $line | cut -d' ' -f 3`
                #echo "$curr_product $curr_pnl $curr_msg_count"
                if [ ${shortcode_pnl[$curr_product]+abc} ] ; then
                        shortcode_pnl[$curr_product]=`echo "${shortcode_pnl[$curr_product]} + $curr_pnl" | bc`
                        shortcode_msg_count[$curr_product]=`echo "${shortcode_msg_count[$curr_product]} + $curr_msg_count" | bc`
                else
                        shortcode_pnl[$curr_product]=$curr_pnl
                        shortcode_msg_count[$curr_product]=$curr_msg_count
                fi
        done < $Monthly_stratFile

        cp $Monthly_stratFile $Monthly_stratFile_backup
        >$Monthly_stratFile
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th></tr></thead><tbody>">> $Monthly_Top_Gainer_Loser ;
        for k in "${!shortcode_pnl[@]}"
        do
                  curr_product=$k
                  curr_pnl=${shortcode_pnl["$k"]}
                  curr_msg_count=${shortcode_msg_count["$k"]}
                  [ -z "$curr_msg_count" ] && curr_msg_count=0;
                  echo "$curr_product $curr_pnl $curr_msg_count"
                  echo "<tr><td>$curr_product</td><td>$curr_pnl</td><td>$curr_msg_count</td></tr>" >> $Monthly_Top_Gainer_Loser;
                  echo "$curr_product $curr_pnl $curr_msg_count" >> $Monthly_stratFile
        done
        cat $Path"html/pnl_report_footer.txt" >> $Monthly_Top_Gainer_Loser; 
        echo "DONE: Get Top Gainer Monthly , file $Monthly_Top_Gainer_Loser"
}

Get_Year_Top_Gainers_Losers (){
        echo "Get Top Gainer Yearly , file ${REPORTS_DEST_DIR}${server}/${yyyy}_top_gainers_losers.html"
        declare -A shortcode_pnl
        declare -A shortcode_msg_count
        declare -A shortcode_conf_count
        declare -A shortcode_cxld_count
        declare -A shortcode_cxre_count
        declare -A shortcode_OF_0_count
        declare -A shortcode_OF_1_count

        shortcode_pnl=()
        shortcode_msg_count=()
        shortcode_conf_count=()
        shortcode_cxld_count=()
        shortcode_cxre_count=()
        shortcode_OF_0_count=()
        shortcode_OF_1_count=()

        Year_Top_Gainer_Loser="${REPORTS_DEST_DIR}${server}/${yyyy}_top_gainers_losers.html" ;
        cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS year ['$yyyy' ] /g' > $Year_Top_Gainer_Loser ;
        while read line 
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                curr_pnl=`echo $line | cut -d' ' -f 2`
                curr_msg_count=`echo $line | cut -d' ' -f 3`
                if [ ${shortcode_pnl[$curr_product]+abc} ] ; then
                        shortcode_pnl[$curr_product]=`echo "${shortcode_pnl[$curr_product]} + $curr_pnl" | bc`
                        shortcode_msg_count[$curr_product]=`echo "${shortcode_msg_count[$curr_product]} + $curr_msg_count" | bc`
                else
                        shortcode_pnl[$curr_product]=$curr_pnl
                        shortcode_msg_count[$curr_product]=$curr_msg_count
                fi
        done < $Yearly_stratFile

        cp $Yearly_stratFile $Yearly_stratFile_backup
        >$Yearly_stratFile
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th></tr></thead><tbody>">> $Year_Top_Gainer_Loser ;
        for k in "${!shortcode_pnl[@]}"
            do
                  curr_product=$k
                  curr_pnl=${shortcode_pnl["$k"]}
                  curr_msg_count=${shortcode_msg_count["$k"]}
                  [ -z "$curr_msg_count" ] && curr_msg_count=0;

                  echo "$curr_product $curr_pnl $curr_msg_count"
                  echo "<tr><td>$curr_product</td><td>$curr_pnl</td></td></tr>" >> $Year_Top_Gainer_Loser;
                  echo "$curr_product $curr_pnl $curr_msg_count" >> $Yearly_stratFile

        done
        cat $Path"html/pnl_report_footer.txt" >> $Year_Top_Gainer_Loser;
        echo "Done: Top Gainer Yearly , file $Year_Top_Gainer_Loser"
}

Compute_Daily_PNL() {
 echo "Compute_Daily_PNL";
 declare -a stratfiles1=();
 declare -a stratfiles2=();

 for server in $servers;
  do
        cd ${SourceDir}${server}
	echo -e "$PWD\n"
	echo -e "for $server\n"
        Yearly_stratFile="${PNLDataPath}${server}/${yyyy}_YearlyProductStrat" ;
        Yearly_stratFile_backup="${PNLDataPath}${server}/${yyyy}_YearlyProductStrat_backup" ;
        Monthly_stratFile="${PNLDataPath}${server}/${yyyy}${mm}_MonthlyProductStrat" ;
        Monthly_stratFile_backup="${PNLDataPath}${server}/${yyyy}${mm}_MonthlyProductStrat_backup" ;
        Daily_stratFile="${PNLDataPath}${server}/${yyyy}${mm}${dd}_DailyProductStrat" ;
        Daily_stratFile_backup="${PNLDataPath}${server}/${yyyy}${mm}${dd}_DailyProductStrat_backup" ;
        for file in `ls | grep "log.$date.*"`;
          do
		      stratNo=$(echo $file| cut -d'.' -f3);
          date_log=$(echo $file | cut -d'.' -f2);
   		    echo -e "\t $server file, stratNo => $file, $stratNo\n";
		      
          if ! `grep -Fxq "${date}_stratID_${stratNo}" $file_list`;
		      then
		         echo -e "${date}_stratID_${stratNo} not present\n" 	
		         echo "${date}_stratID_${stratNo}" >> $file_list
		         stratfiles1+=("${date}_stratID_${stratNo}");

		         mkdir -p "${PNLDataPath}${server}"
		         echo -e "before PNL\n"
             totalPNL=`cat $file | grep PORTFOLIO | awk '{print $3}'`
             totalNumOpen=`cat $file | grep PORTFOLIO | awk '{print $11}'`
		         echo -e "StratNO Pnl, Numopen::$stratNo $totalPNL, $totalNumOpen\n"
             echo "$totalPNL $totalNumOpen" > "${PNLDataPath}${server}/${yyyy}${mm}${dd}_stratID_${stratNo}";
		      fi
                
          if [[ " ${stratfiles2[*]} " == *"$date_log.$stratNo"* ]] ; then
              continue;
          fi
          stratfiles2+=("$date_log.$stratNo");
          
          Daily_Top_Gainer_Loser_FILE=${REPORTS_DEST_DIR}${server}"/"${yyyy}${mm}${dd}"_"${stratNo}"_top_gainers_losers.html" ;
          if [ $server == "IND14" ] || [ $server == "IND15" ] || [ $server == "IND19" ] || [ $server == "IND20" ] ; then
              echo "${server}:SIMRESULT:${file}"
              products=`cat $file | grep SIMRESULT | awk '{print $3,$5,$19;}' | tac | awk '!seen[$1]++' | sed "s#NSE_##g" | sed "s#_[^ ]* # #g" | awk '{pnl[$1]+=$2;msg[$1]+=$3;} END {for (key in pnl) {print "NSE_"key,pnl[key],msg[key];}}' | sort -n -k2`
              echo "$products" | grep -v '^$' >> $Daily_stratFile 
              echo "$products" | grep -v '^$' >> $Monthly_stratFile 
              echo "$products" | grep -v '^$' >> $Yearly_stratFile 
              #products=`cat $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
          else
              echo "${server}:SIMRESULT:${file}"
              products=`cat $file | grep SIMRESULT | awk '{print $3,$5,$19}' | tac | awk '!seen[$1]++' | sort -n -k2`
              echo "$products" | grep -v '^$' >> $Daily_stratFile
              echo "$products" | grep -v '^$' >> $Monthly_stratFile
              echo "$products" | grep -v '^$' >> $Yearly_stratFile 
              #products=`cat $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
          fi
          Get_Curr_Top_Gainers_Losers $* 
        done
        Get_Daily_Top_Gainers_Losers $*
        Get_Month_Top_Gainers_Losers $*
        Get_Year_Top_Gainers_Losers $*
  done
echo -e "stratfiles1=> ${stratfiles1[@]}\n"

 for server in $servers;
 do
    echo "path:-  $PNLDataPath$server"
    [ -d "$PNLDataPath$server" ] || continue 
    cd $PNLDataPath$server
    echo -e "$PWD\n"
    echo -e "for $server\n"
    echo "${PNLDataPath}${server}/${yyyy}${mm}${dd}_dailyStratPNL"
    > "${PNLDataPath}${server}/${yyyy}${mm}${dd}_dailyStratPNL";
    for stratID_file in `ls | grep "${yyyy}${mm}${dd}_stratID_"`;
    do
        stratID=`echo "$stratID_file" | cut -d'_' -f3`
        echo -e "****stratNo=> $stratID\n"
	      if [[ " ${stratfiles1[*]} " == *"$stratID_file"* ]]; then
           TotalMonthlystratfile="${PNLDataPath}${server}/${yyyy}${mm}_TotalMonthlyStratPNL"
           Monthlystratfile="${PNLDataPath}${server}/${yyyy}${mm}_monthlyStratPNL_${stratID}"
           echo -e  "cat:: $stratID_file\n"
           stratPNL=`cat $stratID_file | cut -d' ' -f1`
           stratNumOpen=`cat $stratID_file | cut -d' ' -f2`
	         [ -z $stratPNL ] && stratPNL=0
           [ -z $stratNumOpen ] && stratNumOpen=0
           echo $stratPNL $stratNumOpen
	         echo -e "daily=> stratID, NUMopen, totalpnl, totalNumopen:: $stratPNL, $stratNumOpen, $totalPNL, $totalNumOpen\n\n";
           echo $server $server
           echo "$stratID $stratPNL $stratNumOpen"
  echo; echo "********************************************************************************************************************************************************"; echo ;
      	   echo "$stratID $stratPNL $stratNumOpen" >> "${PNLDataPath}${server}/${yyyy}${mm}${dd}_dailyStratPNL";
           Per_Day_PNLReports $*
  echo; echo "********************************************************************************************************************************************************"; echo ;
           if `grep -q $stratID $Monthlystratfile`;
           then
              lineToReplace=`grep ${yyyy}${mm}${dd} ${Monthlystratfile} | head -1`
              replacement=$yyyy$mm$dd" "$stratPNL" "$stratNumOpen
              sed -i "s/${lineToReplace}/${replacement}/" $Monthlystratfile
           else
              echo "$yyyy$mm$dd $stratPNL $stratNumOpen" >> "${PNLDataPath}${server}/${yyyy}${mm}_monthlyStratPNL_${stratID}";
 	         fi

 	      `sort "${PNLDataPath}${server}/${yyyy}${mm}_monthlyStratPNL_${stratID}" | uniq > "/tmp/temp"; mv "/tmp/temp" "${PNLDataPath}${server}/${yyyy}${mm}_monthlyStratPNL_${stratID}"`

      	 totalPNL=`cat "${PNLDataPath}${server}/${yyyy}${mm}_monthlyStratPNL_${stratID}" | awk '{t+=$2} END {print t}'`
      	 totalNumOpen=`cat "${PNLDataPath}${server}/${yyyy}${mm}_monthlyStratPNL_${stratID}" | awk '{t+=$3} END {print t}'`
      	 echo -e "StratID, PNL, NumOpen:: $stratID,\n $totalPNL,\n $totalNumOpen \n"

       	 if `grep -q $stratID $TotalMonthlystratfile`;
         then
            lineToReplace=`grep $stratID $TotalMonthlystratfile | head -1`
            replacement=$stratID" "$totalPNL" "$totalNumOpen
            sed -i "s/${lineToReplace}/${replacement}/" $TotalMonthlystratfile
          else 
            echo -e "$stratID $totalPNL $totalNumOpen" >> $TotalMonthlystratfile ;
          fi
	      fi 
	   done
	#compute Yearly
	   declare -A yy_stratItradern
     declare -A yy_stratID_pnl
	   yy_stratID_open=()
     yy_stratID_pnl=()
	   cd ${PNLDataPath}${server}
	   echo "YEar data"
	   for monthly_strat_file in `ls | grep ${yyyy} | grep "TotalMonthlyStratPNL"`;
	   do 
		     while read line 
		     do
		      	stratID=`cut -d' ' -f1 <<< $line`
            totalPNL=`cut -d' ' -f2 <<< $line`
			      totalNumOpen=`cut -d' ' -f3 <<< $line`
			      if [ ${yy_stratID_open[$stratID]+abc} ] ; then
                ((yy_stratID_pnl[$stratID]+=$totalPNL))
                ((yy_stratID_open[$stratID]+=$totalNumOpen))
                        else
                                yy_stratID_pnl[$stratID]=$totalPNL
                                yy_stratID_open[$stratID]=$totalNumOpen
                        fi
		done < "$monthly_strat_file"
	    done
	    Yearlystratfile="${PNLDataPath}${server}/${yyyy}_YearlyStratPNL"
            TotalYearlystratfile="${PNLDataPath}${server}/${yyyy}_TotalYearlyStratPNL"
       	    total_year_pnl=0
            total_year_open=0
	    >$Yearlystratfile
            for stratID in "${!yy_stratID_pnl[@]}"; do
             	echo -e "$stratID ${yy_stratID_pnl[$stratID]} ${yy_stratID_open[$stratID]}" >> $Yearlystratfile ;
            	((total_year_pnl+=${yy_stratID_pnl[$stratID]}))
    		((total_year_open+=${yy_stratID_open[$stratID]}))
       	    done
            echo -e "$total_year_pnl $total_year_open" > $TotalYearlystratfile ;
  done
  echo "DONE: Compute_Daily_PNL";
}


EachMonthPNLReports () {
  #server=$1 ;
  mkdir -p $REPORTS_DEST_DIR$server
  EachYearPNL_backup_FILE=${REPORTS_DEST_DIR}${server}"/"$server"_eachYearPNL_backup" ;
  EachMonthPNL_backup_FILE=${REPORTS_DEST_DIR}${server}"/"$server"_eachMonthPNL_backup" ;
  EachDayPNL_backup_FILE=${REPORTS_DEST_DIR}${server}"/"$server"_eachDayPNL_backup" ;
  PerMonthPNL_FILE=${REPORTS_DEST_DIR}${server}"/"${server}"_eachMonthPNL_index.html"
  > $PerMonthPNL_FILE
  # yearly data generated
  cat $Path"html/pnl_report_header2.txt" | sed 's/PNL REPORTS/PNL REPORTS Per YEAR [ '$server' ] /g' >> $PerMonthPNL_FILE ;
  echo "<table class="table table-striped" style='border: 1px solid grey;margin-bottom: 0px;'><thead><tr>" >> $PerMonthPNL_FILE
  echo "<th>Year</th><th>Graph</th><th>TotalPNL</th></tr></thead><tbody>" >> $PerMonthPNL_FILE
  cd $Path"SimPNLCount1/"$server ;

  grep -v ">$yyyy<" $EachYearPNL_backup_FILE > /tmp/${yyyy}PNL_tmp 
  cat /tmp/${yyyy}PNL_tmp > $EachYearPNL_backup_FILE
  Y_PNL="${yyyy}_TotalYearlyStratPNL";
  totalPnl=`awk '{t+=$1} END {print t}' $Y_PNL`
  hrefImg=${server}"_"${yyyy}".jpeg"
  GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'${REPORTS_DEST_DIR}${server}'/'$server'_'$yyyy'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$Path'SimPNLCount1/'$server'/'$yyyy'_YearlyStratPNL" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'
  echo $GNUPLOT | gnuplot ;
  echo "<tr><td><a href=$yyyy"_top_gainers_losers.html" style="color:blue">$yyyy</a></td><td><a href=$hrefImg><img border=0 src='../Graph-512.png' width=40 height=25></a></td><td><a href=$yyyy"_index.html">$totalPnl</a></td></tr>" >>$EachYearPNL_backup_FILE ;
  cat $EachYearPNL_backup_FILE >> $PerMonthPNL_FILE

  # monthly data
  echo "</tbody></table>" >> $PerMonthPNL_FILE
  echo "<div class='row header' style='text-align:center;color:green'><h3>PNL REPORTS PER MONTH [ $server ]</h3></div>" >> $PerMonthPNL_FILE
  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $PerMonthPNL_FILE
  echo "<th>Date</th><th>Graph</th><th>TotalPNL</th></tr></thead><tbody>" >> $PerMonthPNL_FILE

#  echo -e "Server,PWD:: $server, $PWD\n "
  grep -v ">$yyyy$mm<" $EachMonthPNL_backup_FILE > /tmp/${yyyy}${mm}PNL_tmp
  cat /tmp/${yyyy}${mm}PNL_tmp > $EachMonthPNL_backup_FILE
  monthlyPNL="${yyyy}${mm}_TotalMonthlyStratPNL"
  hrefImg=${server}"_"${yyyy}${mm}".jpeg"
  GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy$mm'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'$REPORTS_DEST_DIR$server'/'${server}'_'${yyyy}${mm}'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$Path'SimPNLCount1/'$server'/'$yyyy$mm'_TotalMonthlyStratPNL" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'

  echo $GNUPLOT | gnuplot ;
  echo "$server $yyyy $mm"
  totalPnl=`awk '{t+=$2} END {print t}' $monthlyPNL`
  totalNewOpen=`awk '{o+=$3} END {print o}' $monthlyPNL`
  echo -e "  date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen \n"
  hrefRef="${yyyy}${mm}_index.html" ;
  echo "<tr><td><a href=${yyyy}${mm}"_top_gainers_losers.html" style="color:blue">$yyyy$mm</a></td><td><a href=$hrefImg><img border=0 src='../Graph-512.png' width=40 height=25></a></td><td><a href=$hrefRef>$totalPnl</a></td></tr>" >>$EachMonthPNL_backup_FILE ;
  cat $EachMonthPNL_backup_FILE >>$PerMonthPNL_FILE ;

  # daily data
  echo "</tbody></table>" >> $PerMonthPNL_FILE
  echo "<div class='row header' style='text-align:center;color:green'><h3>PNL REPORTS PER DAY [ $server ]</h3></div>" >> $PerMonthPNL_FILE
  echo "<table id='myTable2' class='table table-striped' ><thead><tr>" >> $PerMonthPNL_FILE
  echo "<th>Date</th><th>TotalPNL</th></tr></thead><tbody>" >> $PerMonthPNL_FILE

#  echo -e "Server,PWD:: $server, $PWD\n "
  grep -v ">$yyyy$mm$dd<" $EachDayPNL_backup_FILE > /tmp/${yyyy}${mm}${dd}PNL_tmp
  cat /tmp/${yyyy}${mm}${dd}PNL_tmp > $EachDayPNL_backup_FILE
  dailyPNL="${yyyy}${mm}${dd}_dailyStratPNL";
  echo "$server $yyyy $mm $dd"
  totalPnl=`awk '{t+=$2} END {print t}' $dailyPNL`
  totalNewOpen=`awk '{o+=$3} END {print o}' $dailyPNL`
  echo -e "  date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen \n"
  echo "<tr><td><a href=${yyyy}${mm}${dd}"_top_gainers_losers.html" style="color:blue">$yyyy$mm$dd</a></td><td>$totalPnl</td></tr>" >>$EachDayPNL_backup_FILE ;
  cat $EachDayPNL_backup_FILE >>$PerMonthPNL_FILE ;

  # echo "</tbody></table></body></html>" >>$PerMonthPNL_FILE ;
  cat $Path"html/pnl_report_footer.txt" >>$PerMonthPNL_FILE ;
}


SpecificYearPNLReports () {
      #server=$1 ;
      cd $Path"SimPNLCount1/"$server
      echo -e "Specific Year($server:$yyyy)::\n"
      file="${yyyy}_YearlyStratPNL"
      SpecificYearPNL_FILE="$REPORTS_DEST_DIR$server/${yyyy}_index.html" ;
      echo "Year file" $SpecificYearPNL_FILE
      mkdir -p ${REPORTS_DEST_DIR}${server} ;
      >$SpecificYearPNL_FILE
      cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS [ '$yyyy' ] /g' > $SpecificYearPNL_FILE ;
      echo "<th>STRATEGY_ID</th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificYearPNL_FILE ;
      while read line
      do
        stratid=`echo $line | cut -d' ' -f 1`
        pnl=`echo $line | cut -d' ' -f 2`
        numOpen=`echo $line | cut -d' ' -f 3`
        echo -e "       stratid:: $stratid\n"
        #`awk '{i+=$2} {print $1,i}' $Path'SimPNLCount1/'$server'/'$yyyy'/'$mm'/monthlyStratPNL.'$stratid`
        echo "<tr><td>$stratid</td><td>$pnl</td></tr>" >> $SpecificYearPNL_FILE ;
      done < $file
#      echo "</tbody></table></body></html>" >> 
      cat $Path"html/pnl_report_footer.txt" >> $SpecificYearPNL_FILE ;
}

SpecificMonthPNLReports () {
    #server=$1 ;
    cd ${Path}"SimPNLCount1/"${server}
        echo -e "Specific month($server:$yyyy$mm)::\n"
#       echo -e "       yearmm, pwd:: $yyyy$mm, $PWD\n"

#         echo -e "file:: $file\n"
      file="${yyyy}${mm}_TotalMonthlyStratPNL"
      SpecificMonthPNL_FILE="${REPORTS_DEST_DIR}${server}/${yyyy}${mm}_index.html" ;
#      SpecificMonthPNL_backup_FILE=$Path"html/"$server"/"$yyyy"/"$mm"/"$yyyy$mm".backup" ;

      mkdir -p ${REPORTS_DEST_DIR}${server} ;
      >$SpecificMonthPNL_FILE
      cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS [ '$yyyy$mm' ] /g' > $SpecificMonthPNL_FILE ;
      echo "<th>STRATEGY_ID</th><th>Graph</th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificMonthPNL_FILE ;


      while read line
      do
        stratid=`echo $line | cut -d' ' -f 1`
        pnl=`echo $line | cut -d' ' -f 2`
        numOpen=`echo $line | cut -d' ' -f 3`
        hrefRef="${yyyy}${mm}_${stratid}_index.html" ;
        echo -e "       stratid:: $stratid\n"
        hrefImg="${server}_${stratid}_${yyyy}${mm}.jpeg"

        #`awk '{i+=$2} {print $1,i}' $Path'SimPNLCount1/'$server'/'$yyyy'/'$mm'/monthlyStratPNL.'$stratid`

        GNUPLOT="set terminal jpeg size 1366,768; set xtics rotate; set title '$stratid->$yyyy$mm'; set xlabel 'Date'; set ylabel 'PNL count'; set output '${REPORTS_DEST_DIR}${server}/${server}_${stratid}_${yyyy}${mm}.jpeg'; set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 pi -1 ps 1.5; set pointintervalbox 3; set grid; plot \"<awk '{i+=\$2} {print \$1,i}'  $Path'SimPNLCount1/'${server}/${yyyy}${mm}_monthlyStratPNL_$stratid\" u (column(0)):2:xtic(1) with linespoints ls 1 title ''"

        echo $GNUPLOT | gnuplot ;

        echo "<tr><td>$stratid</td><td><a href=$hrefImg><img border=0 src='../../../Graph-512.png' width=40 height=25></a></td><td><a href=$hrefRef>$pnl</a></td></tr>" >> $SpecificMonthPNL_FILE ;
      done < $file
#      echo "</tbody></table></body></html>" >> 
      cat $Path"html/pnl_report_footer.txt" >> $SpecificMonthPNL_FILE ;
}


SpecificStrategyMonthlyPNLReports () {
   # server=$1;
        cd ${Path}"SimPNLCount1/"${server}
        #echo -e "month, pwd:: $mm, $PWD\n"
        for file in `ls | grep "${yyyy}${mm}_monthlyStratPNL_"` ;
        do
          stratid=`echo "$file" | cut -d'_' -f 3`
        #  echo -e "    stratID::$file,  $stratid, $PWD\n"
          SpecificStrategyMonthPNL_FILE="${REPORTS_DEST_DIR}${server}/${yyyy}${mm}_${stratid}_index.html" ;
#         mkdir -p $Path"html/"$server"/"$yyyy"/"$mm ;
          >$SpecificStrategyMonthPNL_FILE
          cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' PNL REPORTS For Strategy_ID '$stratid' [ '$yyyy$mm' ] /g' > $SpecificStrategyMonthPNL_FILE ;
          echo "<th>Date</th><th>PNL_COUNT</th></tr></thead><tbody>">> $SpecificStrategyMonthPNL_FILE ;
          while read line
          do
            date_=`echo $line | cut -d' ' -f 1`
            pnl=`echo $line | cut -d' ' -f 2`
            yyyy_=${date_:0:4};
            mm_=${date_:4:2};
            dd_=${date_:6:2};
            echo "<tr><td><a href="${yyyy_}${mm_}${dd_}_${stratid}_top_gainers_losers.html" style="color:blue">$date_</a></td><td>$pnl</td></tr>" >> $SpecificStrategyMonthPNL_FILE ;
          done < $file
#         echo "</tbody></table></body></html>" >> 
          cat $Path"html/pnl_report_footer.txt" >> $SpecificStrategyMonthPNL_FILE ;
        done
}


Server_PNLReports() {
  echo "Server_PNLReports"
  declare -a lastFiveDates=();
  >$SERVER_REPORTING_FILE ;
  #HEADER
  cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/SIM PNL REPORTS /g' > $SERVER_REPORTING_FILE ;
  echo; echo;
  echo "<th>Date</th>" >> $SERVER_REPORTING_FILE;
  for server in $servers;
  do
    echo "<th><a href=$server"/"$server"_eachMonthPNL_index.html" style="color:red">$server</a></th>" >> $SERVER_REPORTING_FILE ;
    EachMonthPNLReports ;
    SpecificMonthPNLReports ;
    SpecificStrategyMonthlyPNLReports ;
    SpecificYearPNLReports ;
  done
  echo "</tr></thead><tbody>" >> $SERVER_REPORTING_FILE ;
  i=-1;
  for (( c=1; c<31 ; c++ ))
  do
   i=$((i + 1))
   temp=`date --date="$yyyymmdd -$i day" +%a`
   tempdate=`date --date="$yyyymmdd -$i day" +%Y%m%d`
   is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $tempdate T`
   while [ "$temp" = "Sun" ] || [ "$is_holiday" = "1" ]
   do
     i=$((i + 1))
     temp=`date --date="$yyyymmdd -$i day" +%a`
     tempdate=`date --date="$yyyymmdd -$i day" +%Y%m%d`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $tempdate T`
   done
   temp=`date --date="$yyyymmdd -$i day" +\%Y\%m\%d`
   lastFiveDates+=("$temp")
   done 
  for day in ${lastFiveDates[@]}
  do
    echo "<tr><td>$day</td>" >>$SERVER_REPORTING_FILE;
    for server in $servers;
    do
      yyyy=${day:0:4};
      mm=${day:4:2};
      dd=${day:6:2};

      if [ -f "${Path}SimPNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL" ]; then
       totalpnl=`awk '{t+=$2} END {print t}' "${Path}SimPNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL"`
      echo "<td><a href="$server"/"${server}"_"${day}"_index.html" style='color:blue'">$totalpnl</a></td>" >> $SERVER_REPORTING_FILE ;
      else
          echo "<td> </td>" >> $SERVER_REPORTING_FILE ;
      fi
    done
    echo "</tr><tr><td style='visibility:hidden'>$day</td>" >>$SERVER_REPORTING_FILE;
    for server in $servers;
     do
       yyyy=${day:0:4};
      mm=${day:4:2};
      dd=${day:6:2};
       ssh dvcinfra@10.23.5.67 [[ -f "/home/dvcinfra/PNLProject/PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL" ]] && livefileexist=1 || livefileexist=0
      if [[ $livefileexist -eq 1 ]];then
       livetotalpnl=`ssh dvcinfra@10.23.5.67 "awk '{t+=\\$2} END {print t}' '/home/dvcinfra/PNLProject/PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL'"`
       echo "<td>$livetotalpnl</td>" >> $SERVER_REPORTING_FILE ;
      else
          echo "<td> </td>" >> $SERVER_REPORTING_FILE ;
      fi
    done
    echo "</tr>" >> $SERVER_REPORTING_FILE;
  done
  cat $Path"html/pnl_report_footer.txt" >> $SERVER_REPORTING_FILE;
  echo "DONE: Server_PNLReports , file $SERVER_REPORTING_FILE"
}

Compute_Daily_Data() {
  date=$2;
  if [ "$2" == "YESTERDAY" ] ; then
    date=`date -d "1 day ago" +"%Y%m%d"` ;
  fi

  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`

  if [ $is_holiday = "1" ]; then
    echo "holiday $date"
    exit -1
  fi

  echo -e "date=> $date\n"
  yyyymmdd=$date;
  yyyy=${date:0:4};
  mm=${date:4:2};
  dd=${date:6:2};
  
  Compute_Daily_PNL $*
  Server_PNLReports $*
  chmod -R +777 $REPORTS_DEST_DIR
}

init () {
  echo "number of arg:: $# ";
  [ $# -eq 2 ] || print_msg_and_exit "Usage : < script > < TYPE(DAILY)> <(YESTERDAY/YYYYMMDD)>"
  OPTION=$1;
  case $OPTION in
    DAILY)
      Compute_Daily_Data $*
      ;;
    *)
      print_msg_and_exit "Usage : < script > < TYPE(DAILY) > <(YESTERDAY/YYYYMMDD)>";
  esac
}

init $*
