#!/bin/bash

print_msg_and_exit (){
 echo $1;
 exit
}

servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20 IND23 IND24" ;
#servers="IND14" ;
SourceDir="/home/hardik/PNLProject/copy/"
Local42ThrottleFilesDir="/home/raghu/ThrottleProject/product_throttle_generated/"
Local42Generate_Throttle_Exec="/home/raghu/ThrottleProject/mds_log_reader_throttle_pnl"
Path="/home/hardik/PNLProject/";
#Path="/home/hardik/PNLProject/test/msgcount/";
PNLDataPath="/home/hardik/PNLProject/PNLCount1/";
#PNLDataPath="/home/hardik/PNLProject/test/msgcount/PNLCount1/";
ThrottleFilesDir="/home/hardik/PNLProject/product_throttle_generated/";
#ThrottleFilesDir="/home/hardik/PNLProject/test/msgcount/product_throttle_generated/";
REPORTS_DEST_DIR="/NAS1/data/PNLReportsIND/www/PNLReportsIND/";
#REPORTS_DEST_DIR="/NAS1/data/PNLReportsIND/www/PNLReportsTest/";
SERVER_REPORTING_FILE=$REPORTS_DEST_DIR"/index.html" ;
#LAT_FILES_DIR="/var/www/html/LatencyReports/";

file_list="/home/hardik/PNLProject/file_list.txt"
#file_list="/home/hardik/PNLProject/test/msgcount/file_list.txt"

declare -A ServerMap=([IND15]=305 [IND16]=308 [IND17]=307 [IND18]=306 [IND14]=314 [IND20]=320 [IND19]=340 [IND23]=312 [IND24]=344)

declare -a day_a=(01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31)
declare -a month_a=(01 02 03 04 05 06 07 08 09 10 11 12)
declare -a year_a=(2020 2021)

sync_throttle_verify() {
 echo "syncing ${Local42ThrottleFilesDir}${yyyymmdd} to $ThrottleFilesDir"
 rsync -ravz 10.23.5.42:${Local42ThrottleFilesDir}${yyyymmdd} $ThrottleFilesDir
 [ ! -d ${ThrottleFilesDir}${yyyymmdd} ] && ssh 10.23.5.42 "/home/raghu/ThrottleProject/mds_log_reader_overall_product_throttle GEN_THROTTLE ${yyyymmdd} >/home/raghu/ThrottleProject/output_overall_product_throttle" && rsync -ravz 10.23.5.42:${Local42ThrottleFilesDir}${yyyymmdd} $ThrottleFilesDir 
 echo "sync DONE"
}

Compute_Daily_Throttle() {
  echo "Compute_Daily_Throttle $server"
  server_base=${ServerMap[$server]}
  for curr_product in `ls ${ThrottleFilesDir}${yyyy}${mm}${dd}/ | awk -v server=$server_base -F'_' '{if($1 == server) print $2"_"$3}' | sort | uniq`
  do
    conf_count=0;cxld_count=0;cxre_count=0;throttle_count=0;OF_0_count=0;OF_1_count=0;
    for throttle_file in `ls ${ThrottleFilesDir}${yyyy}${mm}${dd}/ | grep "${server_base}_" | grep "_${curr_product}_"`
    do
      ((conf_count+=`cat ${ThrottleFilesDir}${yyyy}${mm}${dd}/$throttle_file | cut -d' ' -f2`))
      ((cxld_count+=`cat ${ThrottleFilesDir}${yyyy}${mm}${dd}/$throttle_file | cut -d' ' -f4`))
      ((cxre_count+=`cat ${ThrottleFilesDir}${yyyy}${mm}${dd}/$throttle_file | cut -d' ' -f6`))
      ((throttle_count+=`cat ${ThrottleFilesDir}${yyyy}${mm}${dd}/$throttle_file | cut -d' ' -f8`))
      ((OF_0_count+=`cat ${ThrottleFilesDir}${yyyy}${mm}${dd}/$throttle_file | cut -d' ' -f10`))
      ((OF_1_count+=`cat ${ThrottleFilesDir}${yyyy}${mm}${dd}/$throttle_file | cut -d' ' -f12`))
      echo "$curr_product $server_base $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
    done
    echo "$curr_product $server_base $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
    echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count" >> $Throttle_Daily_stratFile
    echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count" >> $Throttle_Yearly_stratFile
    echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count" >> $Throttle_Monthly_stratFile
  done
  echo "Done: Compute_Daily_Throttle $server"
}

Create_Daily_Top_Products_File () {
        echo "Create_Daily_Top_Products_File"
        curr_top_gainer_losers_stratNo_file=$1
        curr_html_file=$2
        curr_date=$3
        echo -e "Create_Daily_Top_Products_File:: $curr_top_gainer_losers_stratNo_file $curr_html_file $curr_date \n";
        cat "/home/hardik/PNLProject/html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS For Strategy_ID '$curr_stratNo' [ '$curr_date' ] /g' > $curr_html_file ;
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th></tr></thead><tbody>">> $curr_html_file ;
        while read line
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                curr_pnl=`echo $line | cut -d' ' -f 2`
                curr_msg_count=`echo $line | cut -d' ' -f 3`
                echo "<tr><td>$curr_product</td><td>$curr_pnl</td><td>$curr_msg_count</td></tr>" >> $curr_html_file;
        done < $curr_top_gainer_losers_stratNo_file
        cat "/home/hardik/PNLProject/html/pnl_report_footer.txt" >> $curr_html_file;
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
  cat $Path"html/pnl_report_header.txt" | sed "s/PNL REPORTS/PNL REPORTS [ ${date} ] /g" >> $PerDayPNL_FILE ;
  echo "<th>STRATEGY_ID</th><th>PNL_COUNT</th></tr></thead><tbody>">> $PerDayPNL_FILE;
  pnl_data_file="${Path}PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL"
  [ -f $pnl_data_file ] || continue
  while read line
    do
         stratId=`echo $line | cut -d' ' -f 1`
         pnlCount=`echo $line | cut -d' ' -f 2`
         numopen=`echo $line | cut -d' ' -f 3`
         echo -e "      stratId, pnlCount, numopen:: $stratId, $pnlCount, $numopen $server\n"
         echo "<tr><td><a href="${yyyy}${mm}${dd}_${stratId}_top_gainers_losers.html" style="color:blue">$stratId</a></td><td>$pnlCount</td></tr>" >> $PerDayPNL_FILE
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
        declare -A shortcode_throttle_count
        declare -A shortcode_OF_0_count
        declare -A shortcode_OF_1_count

        shortcode_pnl=()
        shortcode_msg_count=()
        shortcode_conf_count=()
        shortcode_cxld_count=()
        shortcode_cxre_count=()
        shortcode_throttle_count=()
        shortcode_OF_0_count=()
        shortcode_OF_1_count=()

        Daily_Top_Gainer_Loser="${REPORTS_DEST_DIR}${server}/${yyyy}${mm}${dd}_top_gainers_losers.html" ;
        cat "/home/hardik/PNLProject/html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS Month [ '$mm' ] year ['$yyyy' ] Day [ '$dd' ] /g' > $Daily_Top_Gainer_Loser ;
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

        while read line 
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                conf_count=`echo $line | cut -d' ' -f 2`
                cxld_count=`echo $line | cut -d' ' -f 3`
                cxre_count=`echo $line | cut -d' ' -f 4`
                throttle_count=`echo $line | cut -d' ' -f 5`
                OF_0_count=`echo $line | cut -d' ' -f 6`
                OF_1_count=`echo $line | cut -d' ' -f 7`
                #echo "$curr_product $curr_pnl $curr_msg_count"
                if [ ${shortcode_conf_count[$curr_product]+abc} ] ; then
                        shortcode_conf_count[$curr_product]=`echo "${shortcode_conf_count[$curr_product]} + $conf_count" | bc`
                        shortcode_cxld_count[$curr_product]=`echo "${shortcode_cxld_count[$curr_product]} + $cxld_count" | bc`
                        shortcode_cxre_count[$curr_product]=`echo "${shortcode_cxre_count[$curr_product]} + $cxre_count" | bc`
                        shortcode_throttle_count[$curr_product]=`echo "${shortcode_throttle_count[$curr_product]} + $throttle_count" | bc`
                        shortcode_OF_0_count[$curr_product]=`echo "${shortcode_OF_0_count[$curr_product]} + $OF_0_count" | bc`
                        shortcode_OF_1_count[$curr_product]=`echo "${shortcode_OF_1_count[$curr_product]} + $OF_1_count" | bc`
                else
                        shortcode_conf_count[$curr_product]=$conf_count
                        shortcode_cxld_count[$curr_product]=$cxld_count
                        shortcode_cxre_count[$curr_product]=$cxre_count
                        shortcode_throttle_count[$curr_product]=$throttle_count
                        shortcode_OF_0_count[$curr_product]=$OF_0_count
                        shortcode_OF_1_count[$curr_product]=$OF_1_count
                fi
                echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
        done < $Throttle_Daily_stratFile

        cp $Daily_stratFile $Daily_stratFile_backup
        cp $Throttle_Daily_stratFile $Throttle_Daily_stratFile_backup
        >$Daily_stratFile
        >$Throttle_Daily_stratFile
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th><th>Conf</th><th>Cxld</th><th>CxRe</th><th>Total Throttle</th><th>OF_0</th><th>OF_1</th></tr></thead><tbody>">> $Daily_Top_Gainer_Loser ;
        for k in "${!shortcode_pnl[@]}"
        do
                  curr_product=$k
                  curr_pnl=${shortcode_pnl["$k"]}
                  curr_msg_count=${shortcode_msg_count["$k"]}
                  [ -z "$curr_msg_count" ] && curr_msg_count=0;
                  conf_count=${shortcode_conf_count["$k"]}
                  [ -z "$conf_count" ] && conf_count=0;
                  cxld_count=${shortcode_cxld_count["$k"]}
                  [ -z "$cxld_count" ] && cxld_count=0;
                  cxre_count=${shortcode_cxre_count["$k"]}
                  [ -z "$cxre_count" ] && cxre_count=0;
                  throttle_count=${shortcode_throttle_count["$k"]}
                  [ -z "$throttle_count" ] && throttle_count=0;
                  OF_0_count=${shortcode_OF_0_count["$k"]}
                  [ -z "$OF_0_count" ] && OF_0_count=0;
                  OF_1_count=${shortcode_OF_1_count["$k"]}
                  [ -z "$OF_1_count" ] && OF_1_count=0;
                  echo "$curr_product $curr_pnl $curr_msg_count $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
                  echo "<tr><td>$curr_product</td><td>$curr_pnl</td><td>$curr_msg_count</td><td>$conf_count</td><td>$cxld_count</td><td>$cxre_count</td><td>$throttle_count</td><td>$OF_0_count</td><td>$OF_1_count</td></tr>" >> $Daily_Top_Gainer_Loser;
                  echo "$curr_product $curr_pnl $curr_msg_count" >> $Daily_stratFile
                  echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count" >> $Throttle_Daily_stratFile

        done
        cat "/home/hardik/PNLProject/html/pnl_report_footer.txt" >> $Daily_Top_Gainer_Loser; 
        echo "DONE: Get Top Gainer Daily , file $Daily_Top_Gainer_Loser"
}

Get_Month_Top_Gainers_Losers () {
        echo "Get Top Gainer Monthly , file ${REPORTS_DEST_DIR}${server}/${yyyy}${mm}_top_gainers_losers.html"
        declare -A shortcode_pnl
        declare -A shortcode_msg_count
        declare -A shortcode_conf_count
        declare -A shortcode_cxld_count
        declare -A shortcode_cxre_count
        declare -A shortcode_throttle_count
        declare -A shortcode_OF_0_count
        declare -A shortcode_OF_1_count

        shortcode_pnl=()
        shortcode_msg_count=()
        shortcode_conf_count=()
        shortcode_cxld_count=()
        shortcode_cxre_count=()
        shortcode_throttle_count=()
        shortcode_OF_0_count=()
        shortcode_OF_1_count=()

        Monthly_Top_Gainer_Loser="${REPORTS_DEST_DIR}${server}/${yyyy}${mm}_top_gainers_losers.html" ;
        cat "/home/hardik/PNLProject/html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS Month [ '$mm' ] year ['$yyyy' ] /g' > $Monthly_Top_Gainer_Loser ;
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

        while read line 
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                conf_count=`echo $line | cut -d' ' -f 2`
                cxld_count=`echo $line | cut -d' ' -f 3`
                cxre_count=`echo $line | cut -d' ' -f 4`
                throttle_count=`echo $line | cut -d' ' -f 5`
                OF_0_count=`echo $line | cut -d' ' -f 6`
                OF_1_count=`echo $line | cut -d' ' -f 7`
                #echo "$curr_product $curr_pnl $curr_msg_count"
                if [ ${shortcode_conf_count[$curr_product]+abc} ] ; then
                        shortcode_conf_count[$curr_product]=`echo "${shortcode_conf_count[$curr_product]} + $conf_count" | bc`
                        shortcode_cxld_count[$curr_product]=`echo "${shortcode_cxld_count[$curr_product]} + $cxld_count" | bc`
                        shortcode_cxre_count[$curr_product]=`echo "${shortcode_cxre_count[$curr_product]} + $cxre_count" | bc`
                        shortcode_throttle_count[$curr_product]=`echo "${shortcode_throttle_count[$curr_product]} + $throttle_count" | bc`
                        shortcode_OF_0_count[$curr_product]=`echo "${shortcode_OF_0_count[$curr_product]} + $OF_0_count" | bc`
                        shortcode_OF_1_count[$curr_product]=`echo "${shortcode_OF_1_count[$curr_product]} + $OF_1_count" | bc`
                else
                        shortcode_conf_count[$curr_product]=$conf_count
                        shortcode_cxld_count[$curr_product]=$cxld_count
                        shortcode_cxre_count[$curr_product]=$cxre_count
                        shortcode_throttle_count[$curr_product]=$throttle_count
                        shortcode_OF_0_count[$curr_product]=$OF_0_count
                        shortcode_OF_1_count[$curr_product]=$OF_1_count
                fi
                echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
        done < $Throttle_Monthly_stratFile

        cp $Monthly_stratFile $Monthly_stratFile_backup
        cp $Throttle_Monthly_stratFile $Throttle_Monthly_stratFile_backup
        >$Monthly_stratFile
        >$Throttle_Monthly_stratFile
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th><th>Conf</th><th>Cxld</th><th>CxRe</th><th>Total Throttle</th><th>OF_0</th><th>OF_1</th></tr></thead><tbody>">> $Monthly_Top_Gainer_Loser ;
        for k in "${!shortcode_pnl[@]}"
        do
                  curr_product=$k
                  curr_pnl=${shortcode_pnl["$k"]}
                  curr_msg_count=${shortcode_msg_count["$k"]}
                  [ -z "$curr_msg_count" ] && curr_msg_count=0;
                  conf_count=${shortcode_conf_count["$k"]}
                  [ -z "$conf_count" ] && conf_count=0;
                  cxld_count=${shortcode_cxld_count["$k"]}
                  [ -z "$cxld_count" ] && cxld_count=0;
                  cxre_count=${shortcode_cxre_count["$k"]}
                  [ -z "$cxre_count" ] && cxre_count=0;
                  throttle_count=${shortcode_throttle_count["$k"]}
                  [ -z "$throttle_count" ] && throttle_count=0;
                  OF_0_count=${shortcode_OF_0_count["$k"]}
                  [ -z "$OF_0_count" ] && OF_0_count=0;
                  OF_1_count=${shortcode_OF_1_count["$k"]}
                  [ -z "$OF_1_count" ] && OF_1_count=0;

                  echo "$curr_product $curr_pnl $curr_msg_count $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
                  echo "<tr><td>$curr_product</td><td>$curr_pnl</td><td>$curr_msg_count</td><td>$conf_count</td><td>$cxld_count</td><td>$cxre_count</td><td>$throttle_count</td><td>$OF_0_count</td><td>$OF_1_count</td></tr>" >> $Monthly_Top_Gainer_Loser;
                  echo "$curr_product $curr_pnl $curr_msg_count" >> $Monthly_stratFile
                  echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count" >> $Throttle_Monthly_stratFile
        done
        cat "/home/hardik/PNLProject/html/pnl_report_footer.txt" >> $Monthly_Top_Gainer_Loser; 
        echo "DONE: Get Top Gainer Monthly , file $Monthly_Top_Gainer_Loser"
}

Get_Year_Top_Gainers_Losers (){
        echo "Get Top Gainer Yearly , file ${REPORTS_DEST_DIR}${server}/${yyyy}_top_gainers_losers.html"
        declare -A shortcode_pnl
        declare -A shortcode_msg_count
        declare -A shortcode_conf_count
        declare -A shortcode_cxld_count
        declare -A shortcode_cxre_count
        declare -A shortcode_throttle_count
        declare -A shortcode_OF_0_count
        declare -A shortcode_OF_1_count

        shortcode_pnl=()
        shortcode_msg_count=()
        shortcode_conf_count=()
        shortcode_cxld_count=()
        shortcode_cxre_count=()
        shortcode_throttle_count=()
        shortcode_OF_0_count=()
        shortcode_OF_1_count=()

        Year_Top_Gainer_Loser="${REPORTS_DEST_DIR}${server}/${yyyy}_top_gainers_losers.html" ;
        cat "/home/hardik/PNLProject/html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS year ['$yyyy' ] /g' > $Year_Top_Gainer_Loser ;
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

        while read line 
        do
                curr_product=`echo $line | cut -d' ' -f 1`
                conf_count=`echo $line | cut -d' ' -f 2`
                cxld_count=`echo $line | cut -d' ' -f 3`
                cxre_count=`echo $line | cut -d' ' -f 4`
                throttle_count=`echo $line | cut -d' ' -f 5`
                OF_0_count=`echo $line | cut -d' ' -f 6`
                OF_1_count=`echo $line | cut -d' ' -f 7`
                #echo "$curr_product $curr_pnl $curr_msg_count"
                if [ ${shortcode_conf_count[$curr_product]+abc} ] ; then
                        shortcode_conf_count[$curr_product]=`echo "${shortcode_conf_count[$curr_product]} + $conf_count" | bc`
                        shortcode_cxld_count[$curr_product]=`echo "${shortcode_cxld_count[$curr_product]} + $cxld_count" | bc`
                        shortcode_cxre_count[$curr_product]=`echo "${shortcode_cxre_count[$curr_product]} + $cxre_count" | bc`
                        shortcode_throttle_count[$curr_product]=`echo "${shortcode_throttle_count[$curr_product]} + $throttle_count" | bc`
                        shortcode_OF_0_count[$curr_product]=`echo "${shortcode_OF_0_count[$curr_product]} + $OF_0_count" | bc`
                        shortcode_OF_1_count[$curr_product]=`echo "${shortcode_OF_1_count[$curr_product]} + $OF_1_count" | bc`
                else
                        shortcode_conf_count[$curr_product]=$conf_count
                        shortcode_cxld_count[$curr_product]=$cxld_count
                        shortcode_cxre_count[$curr_product]=$cxre_count
                        shortcode_throttle_count[$curr_product]=$throttle_count
                        shortcode_OF_0_count[$curr_product]=$OF_0_count
                        shortcode_OF_1_count[$curr_product]=$OF_1_count
                fi
                echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
        done < $Throttle_Yearly_stratFile

        cp $Yearly_stratFile $Yearly_stratFile_backup
        cp $Throttle_Yearly_stratFile $Throttle_Yearly_stratFile_backup
        >$Yearly_stratFile
        >$Throttle_Yearly_stratFile
        echo "<th>Product</th><th>PNL</th><th>MSG Count</th><th>Conf</th><th>Cxld</th><th>CxRe</th><th>Total Throttle</th><th>OF_0</th><th>OF_1</th></tr></thead><tbody>">> $Year_Top_Gainer_Loser ;
        for k in "${!shortcode_pnl[@]}"
            do
                  curr_product=$k
                  curr_pnl=${shortcode_pnl["$k"]}
                  curr_msg_count=${shortcode_msg_count["$k"]}
                  [ -z "$curr_msg_count" ] && curr_msg_count=0;
                  conf_count=${shortcode_conf_count["$k"]}
                  [ -z "$conf_count" ] && conf_count=0;
                  cxld_count=${shortcode_cxld_count["$k"]}
                  [ -z "$cxld_count" ] && cxld_count=0;
                  cxre_count=${shortcode_cxre_count["$k"]}
                  [ -z "$cxre_count" ] && cxre_count=0;
                  throttle_count=${shortcode_throttle_count["$k"]}
                  [ -z "$throttle_count" ] && throttle_count=0;
                  OF_0_count=${shortcode_OF_0_count["$k"]}
                  [ -z "$OF_0_count" ] && OF_0_count=0;
                  OF_1_count=${shortcode_OF_1_count["$k"]}
                  [ -z "$OF_1_count" ] && OF_1_count=0;

                  echo "$curr_product $curr_pnl $curr_msg_count $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count"
                  echo "<tr><td>$curr_product</td><td>$curr_pnl</td><td>$curr_msg_count</td><td>$conf_count</td><td>$cxld_count</td><td>$cxre_count</td><td>$throttle_count</td><td>$OF_0_count</td><td>$OF_1_count</td></tr>" >> $Year_Top_Gainer_Loser;
                  echo "$curr_product $curr_pnl $curr_msg_count" >> $Yearly_stratFile
                  echo "$curr_product $conf_count $cxld_count $cxre_count $throttle_count $OF_0_count $OF_1_count" >> $Throttle_Yearly_stratFile

        done
        cat "/home/hardik/PNLProject/html/pnl_report_footer.txt" >> $Year_Top_Gainer_Loser;
        echo "Get Top Gainer Yearly , file $Year_Top_Gainer_Loser"
}

Compute_Daily_PNL() {
 echo "Compute_Daily_PNL";
 declare -a stratfiles1=();
 declare -a stratfiles2=();

 sync_throttle_verify
 [ ! -d ${ThrottleFilesDir}${yyyymmdd} ] && echo "${Local42ThrottleFilesDir}${yyyymmdd} NOT PRESENT IN 10.23.5.42"| mailx -s "ALERT : Failed to Generate PNL Report for $yyyymmdd" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in && exit

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
        Throttle_Yearly_stratFile="${PNLDataPath}${server}/${yyyy}_YearlyProductStratThrottle" ;
        Throttle_Yearly_stratFile_backup="${PNLDataPath}${server}/${yyyy}_YearlyProductStratThrottle_backup" ;
        Throttle_Monthly_stratFile="${PNLDataPath}${server}/${yyyy}${mm}_MonthlyProductStratThrottle" ;
        Throttle_Monthly_stratFile_backup="${PNLDataPath}${server}/${yyyy}${mm}_MonthlyProductStratThrottle_backup" ;
        Throttle_Daily_stratFile="${PNLDataPath}${server}/${yyyy}${mm}${dd}_DailyProductStratThrottle" ;
        Throttle_Daily_stratFile_backup="${PNLDataPath}${server}/${yyyy}${mm}${dd}_DailyProductStratThrottle_backup" ;
        for file in `ls -I '*.gz.gz' | grep "log.$date.*"`;
          do
                [[ $server == "IND19" ]] && [[ $file == *12346* ]] && continue;
		stratNo=$(echo $file| cut -d'.' -f 3);
                date_log=$(echo $file | cut -d'.' -f 2);
   		echo -e "\t $server file, stratNo => $file, $stratNo\n";
		if ! `grep -Fxq "${date}_stratID_${stratNo}" $file_list`;
		then
		    echo -e "${date}_stratID_${stratNo} not present\n" 	
		    echo "${date}_stratID_${stratNo}" >> $file_list
		    stratfiles1+=("${date}_stratID_${stratNo}");

		    mkdir -p "${PNLDataPath}${server}"
		     echo -e "before PNL\n"
                     totalPNL=`zless -f $file | grep PORTFOLIO | awk 'END {print $3}'`
                     totalNumOpen=`zless -f $file | grep PORTFOLIO | awk 'END {print $11}'`
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
                   products=`zless $file | grep SIMRESULT | awk '{print $3,$5,$19;}' | tac | awk '!seen[$1]++' | sed "s#NSE_##g" | sed "s#_[^ ]* # #g" | awk '{pnl[$1]+=$2;msg[$1]+=$3;} END {for (key in pnl) {print "NSE_"key,pnl[key],msg[key];}}' | sort -n -k2`
                   echo "$products" | grep -v '^$' >> $Daily_stratFile 
                   echo "$products" | grep -v '^$' >> $Monthly_stratFile 
                   echo "$products" | grep -v '^$' >> $Yearly_stratFile 
                   #products=`zless $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                else
                   echo "${server}:SIMRESULT:${file}"
                   products=`zless $file | grep SIMRESULT | awk '{print $3,$5,$19}' | tac | awk '!seen[$1]++' | sort -n -k2`
                   echo "$products" | grep -v '^$' >> $Daily_stratFile
                   echo "$products" | grep -v '^$' >> $Monthly_stratFile
                   echo "$products" | grep -v '^$' >> $Yearly_stratFile 
                  #products=`zless $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                fi
                Get_Curr_Top_Gainers_Losers $* 
          done
          Compute_Daily_Throttle $*
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
      	   echo "$stratID $stratPNL $stratNumOpen" >> "${PNLDataPath}${server}/${yyyy}${mm}${dd}_dailyStratPNL";
           Per_Day_PNLReports $*
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
	    declare -A yy_stratID_open
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

Compute_Historical_Daily_PNL() {
 exit_date=$date;
 echo "Compute_Historical_Daily_PNL";
 rm -rf $file_list $REPORTS_DEST_DIR"IND"* $PNLDataPath $SERVER_REPORTING_FILE
 for i in "${year_a[@]}"
 do
   for j in "${month_a[@]}"
   do
     for k in "${day_a[@]}"
     do
       date=$i$j$k;
       yyyymmdd=$date;
       yyyy=${date:0:4};
       mm=${date:4:2};
       dd=${date:6:2};

       echo "Compute_Historical_Daily_PNL $date"
       declare -a stratfiles1=();
       declare -a stratfiles2=();

       sync_throttle_verify
       [ ! -d ${ThrottleFilesDir}${yyyymmdd} ] && continue

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
         Throttle_Yearly_stratFile="${PNLDataPath}${server}/${yyyy}_YearlyProductStratThrottle" ;
         Throttle_Yearly_stratFile_backup="${PNLDataPath}${server}/${yyyy}_YearlyProductStratThrottle_backup" ;
         Throttle_Monthly_stratFile="${PNLDataPath}${server}/${yyyy}${mm}_MonthlyProductStratThrottle" ;
         Throttle_Monthly_stratFile_backup="${PNLDataPath}${server}/${yyyy}${mm}_MonthlyProductStratThrottle_backup" ;
         Throttle_Daily_stratFile="${PNLDataPath}${server}/${yyyy}${mm}${dd}_DailyProductStratThrottle" ;
         Throttle_Daily_stratFile_backup="${PNLDataPath}${server}/${yyyy}${mm}${dd}_DailyProductStratThrottle_backup" ;

         for file in `ls -I '*.gz.gz' | grep "log.$date.*"`;
         do
                [[ $server == "IND19" ]] && [[ $file == *12346* ]] && continue;
                stratNo=$(echo $file| cut -d'.' -f 3);
                date_log=$(echo $file | cut -d'.' -f 2);
                echo -e "\t $server file, stratNo => $file, $stratNo\n";
                if ! `grep -Fxq "${date}_stratID_${stratNo}" $file_list`;
                then
                    echo -e "${date}_stratID_${stratNo} not present\n"  
                    echo "${date}_stratID_${stratNo}" >> $file_list
                    stratfiles1+=("${date}_stratID_${stratNo}");

                    mkdir -p "${PNLDataPath}${server}"
                     echo -e "before PNL\n"
                     totalPNL=`zless -f $file | grep PORTFOLIO | awk 'END {print $3}'`
                     totalNumOpen=`zless -f $file | grep PORTFOLIO | awk 'END {print $11}'`
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
                   products=`zless $file | grep SIMRESULT | awk '{print $3,$5,$19;}' | tac | awk '!seen[$1]++' | sed "s#NSE_##g" | sed "s#_[^ ]* # #g" | awk '{pnl[$1]+=$2;msg[$1]+=$3;} END {for (key in pnl) {print "NSE_"key,pnl[key],msg[key];}}' | sort -n -k2`
                   echo "$products" | grep -v '^$' >> $Daily_stratFile
                   echo "$products" | grep -v '^$' >> $Monthly_stratFile
                   echo "$products" | grep -v '^$' >> $Yearly_stratFile
                   #products=`zless $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                else
                   echo "${server}:SIMRESULT:${file}"
                   products=`zless $file | grep SIMRESULT | awk '{print $3,$5,$19}' | tac | awk '!seen[$1]++' | sort -n -k2`
                   echo "$products" | grep -v '^$' >> $Daily_stratFile
                   echo "$products" | grep -v '^$' >> $Monthly_stratFile
                   echo "$products" | grep -v '^$' >> $Yearly_stratFile 
                  #products=`zless $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                fi
                Get_Curr_Top_Gainers_Losers $* 
          done
          Compute_Daily_Throttle $*
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
                echo "$stratID $stratPNL $stratNumOpen" >> "${PNLDataPath}${server}/${yyyy}${mm}${dd}_dailyStratPNL";
                Per_Day_PNLReports $*
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
                 declare -A yy_stratID_open
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
       echo "DONE: Compute_Historical_Daily_PNL $date"
       [[ $date -eq $exit_date ]] && return
      done
    done
  done
echo "DONE: Compute_Historical_Daily_PNL";
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
  cd $Path"PNLCount1/"$server ;

  grep -v ">$yyyy<" $EachYearPNL_backup_FILE > /tmp/${yyyy}PNL_tmp 
  cat /tmp/${yyyy}PNL_tmp > $EachYearPNL_backup_FILE
  Y_PNL="${yyyy}_TotalYearlyStratPNL";
  totalPnl=`awk '{t+=$1} END {print t}' $Y_PNL`
  hrefImg=${server}"_"${yyyy}".jpeg"
  GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'${REPORTS_DEST_DIR}${server}'/'$server'_'$yyyy'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$Path'PNLCount1/'$server'/'$yyyy'_YearlyStratPNL" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'
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
  GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy$mm'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'$REPORTS_DEST_DIR$server'/'${server}'_'${yyyy}${mm}'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$Path'PNLCount1/'$server'/'$yyyy$mm'_TotalMonthlyStratPNL" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'

  echo $GNUPLOT | gnuplot ;
  echo "$server $yyyy $mm"
  totalPnl=`awk '{t+=$2} END {print t}' $monthlyPNL`
  totalNewOpen=`awk '{o+=$3} END {print o}' $monthlyPNL`
  #echo -e "  date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen\n"
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
  #echo -e "  date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen\n"
  echo "<tr><td><a href=${yyyy}${mm}${dd}"_top_gainers_losers.html" style="color:blue">$yyyy$mm$dd</a></td><td>$totalPnl</td></tr>" >>$EachDayPNL_backup_FILE ;
  cat $EachDayPNL_backup_FILE >>$PerMonthPNL_FILE ;

  # echo "</tbody></table></body></html>" >>$PerMonthPNL_FILE ;
  cat $Path"html/pnl_report_footer.txt" >>$PerMonthPNL_FILE ;
}

HistoricalEachMonthPNLReports () {
  #server=$1 ;
  yyyy=$i;
  mm=$j;
  dd=$k;
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
  cd $Path"PNLCount1/"$server ;

  grep -v ">$yyyy<" $EachYearPNL_backup_FILE > /tmp/${yyyy}PNL_tmp 
  cat /tmp/${yyyy}PNL_tmp > $EachYearPNL_backup_FILE
  Y_PNL="${yyyy}_TotalYearlyStratPNL";
  totalPnl=`awk '{t+=$1} END {print t}' $Y_PNL`
  hrefImg=${server}"_"${yyyy}".jpeg"
  GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'${REPORTS_DEST_DIR}${server}'/'$server'_'$yyyy'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$Path'PNLCount1/'$server'/'$yyyy'_YearlyStratPNL" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'
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
  GNUPLOT='set terminal jpeg size 1366,768; set title "'$server'->'$yyyy$mm'"; set xlabel "Strategy ID"; set ylabel "PNL count"; set output "'$REPORTS_DEST_DIR$server'/'${server}'_'${yyyy}${mm}'.jpeg"; set grid;set style line 1 lt 1 lc rgb "green"; set style line 2 lt 1 lc rgb "red"; set style fill solid; plot "'$Path'PNLCount1/'$server'/'$yyyy$mm'_TotalMonthlyStratPNL" u (column(0)):2:(0.5):($2>0?1:2):xtic(1) w boxes lc variable title ""'

  echo $GNUPLOT | gnuplot ;
  echo "$server $yyyy $mm"
  totalPnl=`awk '{t+=$2} END {print t}' $monthlyPNL`
  totalNewOpen=`awk '{o+=$3} END {print o}' $monthlyPNL`
  #echo -e "  date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen\n"
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
  #echo -e "  date, monthlypnl, totalPnl, NumOpen:: $yyyy$dd$mm, $totalPnl, $totalNewOpen\n"
  echo "<tr><td><a href=${yyyy}${mm}${dd}"_top_gainers_losers.html" style="color:blue">$yyyy$mm$dd</a></td><td>$totalPnl</td></tr>" >>$EachDayPNL_backup_FILE ;
  cat $EachDayPNL_backup_FILE >>$PerMonthPNL_FILE ;

  # echo "</tbody></table></body></html>" >>$PerMonthPNL_FILE ;
  cat $Path"html/pnl_report_footer.txt" >>$PerMonthPNL_FILE ;
}

SpecificYearPNLReports () {
      #server=$1 ;
      cd $Path"PNLCount1/"$server
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
        #`awk '{i+=$2} {print $1,i}' $Path'PNLCount1/'$server'/'$yyyy'/'$mm'/monthlyStratPNL.'$stratid`
        echo "<tr><td>$stratid</td><td>$pnl</td></tr>" >> $SpecificYearPNL_FILE ;
      done < $file
#      echo "</tbody></table></body></html>" >> 
      cat $Path"html/pnl_report_footer.txt" >> $SpecificYearPNL_FILE ;
}

HistoricalSpecificYearPNLReports () {
      #server=$1 ;
      yyyy=$i;
      cd $Path"PNLCount1/"$server
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
        #`awk '{i+=$2} {print $1,i}' $Path'PNLCount1/'$server'/'$yyyy'/'$mm'/monthlyStratPNL.'$stratid`
        echo "<tr><td>$stratid</td><td>$pnl</td></tr>" >> $SpecificYearPNL_FILE ;
      done < $file
#      echo "</tbody></table></body></html>" >> 
      cat $Path"html/pnl_report_footer.txt" >> $SpecificYearPNL_FILE ;
}

SpecificMonthPNLReports () {
    #server=$1 ;
    cd ${Path}"PNLCount1/"${server}
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

        #`awk '{i+=$2} {print $1,i}' $Path'PNLCount1/'$server'/'$yyyy'/'$mm'/monthlyStratPNL.'$stratid`

        GNUPLOT="set terminal jpeg size 1366,768; set xtics rotate; set title '$stratid->$yyyy$mm'; set xlabel 'Date'; set ylabel 'PNL count'; set output '${REPORTS_DEST_DIR}${server}/${server}_${stratid}_${yyyy}${mm}.jpeg'; set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 pi -1 ps 1.5; set pointintervalbox 3; set grid; plot \"<awk '{i+=\$2} {print \$1,i}'  $Path'PNLCount1/'${server}/${yyyy}${mm}_monthlyStratPNL_$stratid\" u (column(0)):2:xtic(1) with linespoints ls 1 title ''"

        echo $GNUPLOT | gnuplot ;

        echo "<tr><td>$stratid</td><td><a href=$hrefImg><img border=0 src='../../../Graph-512.png' width=40 height=25></a></td><td><a href=$hrefRef>$pnl</a></td></tr>" >> $SpecificMonthPNL_FILE ;
      done < $file
#      echo "</tbody></table></body></html>" >> 
      cat $Path"html/pnl_report_footer.txt" >> $SpecificMonthPNL_FILE ;
}

HistoricalSpecificMonthPNLReports () {
    #server=$1 ;
    yyyy=$i;
    mm=$j;
    cd ${Path}"PNLCount1/"${server}
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

        #`awk '{i+=$2} {print $1,i}' $Path'PNLCount1/'$server'/'$yyyy'/'$mm'/monthlyStratPNL.'$stratid`

        GNUPLOT="set terminal jpeg size 1366,768; set xtics rotate; set title '$stratid->$yyyy$mm'; set xlabel 'Date'; set ylabel 'PNL count'; set output '${REPORTS_DEST_DIR}${server}/${server}_${stratid}_${yyyy}${mm}.jpeg'; set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 pi -1 ps 1.5; set pointintervalbox 3; set grid; plot \"<awk '{i+=\$2} {print \$1,i}'  $Path'PNLCount1/'${server}/${yyyy}${mm}_monthlyStratPNL_$stratid\" u (column(0)):2:xtic(1) with linespoints ls 1 title ''"

        echo $GNUPLOT | gnuplot ;

        echo "<tr><td>$stratid</td><td><a href=$hrefImg><img border=0 src='../../../Graph-512.png' width=40 height=25></a></td><td><a href=$hrefRef>$pnl</a></td></tr>" >> $SpecificMonthPNL_FILE ;
      done < $file
#      echo "</tbody></table></body></html>" >> 
      cat $Path"html/pnl_report_footer.txt" >> $SpecificMonthPNL_FILE ;
}

SpecificStrategyMonthlyPNLReports () {
   # server=$1;
        cd ${Path}"PNLCount1/"${server}
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
            date=`echo $line | cut -d' ' -f 1`
            pnl=`echo $line | cut -d' ' -f 2`
            yyyy=${date:0:4};
            mm=${date:4:2};
            dd=${date:6:2};
            echo "<tr><td><a href="${yyyy}${mm}${dd}_${stratid}_top_gainers_losers.html" style="color:blue">$date</a></td><td>$pnl</td></tr>" >> $SpecificStrategyMonthPNL_FILE ;
          done < $file
#         echo "</tbody></table></body></html>" >> 
          cat $Path"html/pnl_report_footer.txt" >> $SpecificStrategyMonthPNL_FILE ;
        done
}

HistoricalSpecificStrategyMonthlyPNLReports () {
   # server=$1;
        yyyy=$i;
        mm=$j;
        cd ${Path}"PNLCount1/"${server}
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
            date=`echo $line | cut -d' ' -f 1`
            pnl=`echo $line | cut -d' ' -f 2`
            yyyy=${date:0:4};
            mm=${date:4:2};
            dd=${date:6:2};
            echo "<tr><td><a href="${yyyy}${mm}${dd}_${stratid}_top_gainers_losers.html" style="color:blue">$date</a></td><td>$pnl</td></tr>" >> $SpecificStrategyMonthPNL_FILE ;
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
  cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/PNL REPORTS /g' > $SERVER_REPORTING_FILE ;
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
      if [ -f "${Path}PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL" ]; then
      totalpnl=`awk '{t+=$2} END {print t}' "${Path}PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL"`
      echo "<td><a href="$server"/"${server}"_"${day}"_index.html" style='color:blue'">$totalpnl</a></td>" >> $SERVER_REPORTING_FILE ;
      else
          echo "<td> </td>" >> $SERVER_REPORTING_FILE ;
      fi
    done
    echo "</tr>" >> $SERVER_REPORTING_FILE;
  done
  cat $Path"html/pnl_report_footer.txt" >> $SERVER_REPORTING_FILE;
  echo "DONE: Server_PNLReports , file $SERVER_REPORTING_FILE"
}

Server_PNLReports_Historical() {
  echo "Server_PNLReports_Historical"
  exit_year="$yyyy"
  exit_month="$yyyy$mm"
  exit_day="$yyyy$mm$dd"
  declare -a lastFiveDates=();
  >$SERVER_REPORTING_FILE ;
  #HEADER
  cat $Path"html/pnl_report_header.txt" | sed 's/PNL REPORTS/PNL REPORTS /g' > $SERVER_REPORTING_FILE ;
  echo "<th>Date</th>" >> $SERVER_REPORTING_FILE;
  for server in $servers;
  do
    echo "<th><a href=$server"/"$server"_eachMonthPNL_index.html" style="color:red">$server</a></th>" >> $SERVER_REPORTING_FILE ;
    for i in "${year_a[@]}"
    do
      for j in "${month_a[@]}"
      do 
        for k in "${day_a[@]}"
        do
          holiday_check=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $i$j$k T`
          [[ "$holiday_check" == "1" ]] && continue
          echo "DATE_CHECK: $i$j$k"
          HistoricalEachMonthPNLReports;
          [[ "$i$j$k" == "$exit_day" ]] && break
        done
        HistoricalSpecificMonthPNLReports ;
        HistoricalSpecificStrategyMonthlyPNLReports;
        [[ "$i$j" == "$exit_month" ]] && break
      done
      HistoricalSpecificYearPNLReports
      [[ "$i" == "$exit_year" ]] && break
    done
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
      if [ -f "${Path}PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL" ]; then
      totalpnl=`awk '{t+=$2} END {print t}' "${Path}PNLCount1/${server}/${yyyy}${mm}${dd}_dailyStratPNL"`
      echo "<td><a href="$server"/"${server}"_"${day}"_index.html" style='color:blue'">$totalpnl</a></td>" >> $SERVER_REPORTING_FILE ;
      else
          echo "<td> </td>" >> $SERVER_REPORTING_FILE ;
      fi
    done
    echo "</tr>" >> $SERVER_REPORTING_FILE;
  done
  cat $Path"html/pnl_report_footer.txt" >> $SERVER_REPORTING_FILE;
  echo "DONE: Server_PNLReports_Historical , file $SERVER_REPORTING_FILE"
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

}

Compute_Historical_Data() {
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

  Compute_Historical_Daily_PNL $*
  Server_PNLReports_Historical $*

}

init () {
  echo "number of arg:: $# ";
  [ $# -eq 2 ] || print_msg_and_exit "Usage : < script > < TYPE(DAILY/HISTORICAL> <(YESTERDAY/YYYYMMDD)>"
  OPTION=$1;
  case $OPTION in
    DAILY)
      Compute_Daily_Data $*
      ;;
    HISTORICAL)
      Compute_Historical_Data $*
      ;;
    *)
      print_msg_and_exit "Usage : < script > < TYPE(DAILY/HISTORICAL) > <(YESTERDAY/YYYYMMDD)>";
  esac
}

init $*

