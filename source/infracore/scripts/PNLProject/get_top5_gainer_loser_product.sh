#!/bin/bash

servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20" ;
#servers="IND16" ;
SOURCE_DIR="/home/hardik/PNLProject/copy/"
DEST_DIR="/home/hardik/PNLProject/PNLCount1/"
REPORTS_DEST_DIR="/NAS1/data/PNLReportsIND/www/PNLReportsIND/";
top_gainers="/top_gainers."
top_losers="/top_losers."
Monthly_Dir="/home/hardik/PNLProject/Monthly_strat"
Yearly_Dir="/home/hardik/PNLProject/Monthly_strat"

print_msg_and_exit(){
	echo $1
	exit
}

Create_Daily_Top_Products_File () {
 	curr_src_dir=$1
  	curr_stratNo=$2
  	curr_html_file=$3
  	curr_date=$4
  	echo -e "Create_Daily_Top_Products_File:: $curr_src_dir $curr_stratNo $curr_html_file $curr_date \n";  
	#$DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_losers$stratNo
  	cat "/home/hardik/PNLProject/html/pnl_report_header2.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS For Strategy_ID '$curr_stratNo' [ '$curr_date' ] /g' > $curr_html_file ;
  	echo "<table  class="table table-striped"><thead><tr><th>TOP GAINERS</th><th>PNL</th></tr></thead><tbody>">> $curr_html_file ;  
  	echo "TOP gainers:: ";
  	while read line
  	do
		curr_product=`echo $line | cut -d' ' -f 1`
		curr_pnl=`echo $line | cut -d' ' -f 2`
    		echo "<tr><td>$curr_product</td><td>$curr_pnl</td></tr>" >> $curr_html_file;
  	done < $curr_src_dir$top_gainers$curr_stratNo
  	echo "</tbody><tbody><tr><th>TOP LOSERS</th><th>PNL</th></tr>" >> $curr_html_file;
  	echo "TOP losers:: ";
  	while read line
  	do
		curr_product=`echo $line | cut -d' ' -f 1`
    		curr_pnl=`echo $line | cut -d' ' -f 2`
    		echo "<tr><td>$curr_product</td><td>$curr_pnl</td></tr>" >> $curr_html_file;
  	done < $curr_src_dir$top_losers$curr_stratNo
  	cat "/home/hardik/PNLProject/html/pnl_report_footer.txt" >> $curr_html_file;
  	echo -e "DONE Create_Daily_Top_Products_File::"
}

Get_Curr_Top_Gainers_Losers () {
    echo "Get_Curr_Top_Gainers_Losers";
    mkdir -p $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd
    mkdir -p $REPORTS_DEST_DIR$server"/"$yyyy"/"$mm"/"$dd   
    #top 5 gainers
    #echo -e " top gainers:: \n"
    echo $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_gainers$stratNo
    echo -e "$products\n" | tail -6 | tac  > $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_gainers$stratNo
    #top 5 losers
    #echo -e " top losers:: \n"
    echo -e "$products\n" | head -5 > $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_losers$stratNo
    Create_Daily_Top_Products_File $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd $stratNo $Daily_Top_Gainer_Loser_FILE $date
    echo "DONE Get_Curr_Top_Gainers_Losers"
}

Get_Month_Top_Gainers_Losers () {
	echo "Get Top Gainer Monthly"
	declare -A shortcode_pnl
        shortcode_pnl=()
	Monthly_Top_Gainer_Loser=$REPORTS_DEST_DIR$server"/"$yyyy"/"$mm"/""top_gainers_losers.html" ;
        cat "/home/hardik/PNLProject/html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS Month [ '$mm' ] year ['$yyyy' ] /g' > $Monthly_Top_Gainer_Loser ;
	while read line 
	do 
  		curr_product=`echo $line | cut -d' ' -f 1`
  		curr_pnl=`echo $line | cut -d' ' -f 2`
		if [ ${shortcode_pnl[$curr_product]+abc} ] ; then
			shortcode_pnl[$curr_product]=`echo "${shortcode_pnl[$curr_product]} + $curr_pnl" | bc`
		else
			shortcode_pnl[$curr_product]=$curr_pnl
		fi
  	done < $Monthly_stratFile
  	echo "<th>Product</th><th>PNL</th></tr></thead><tbody>">> $Monthly_Top_Gainer_Loser ;
  	for k in "${!shortcode_pnl[@]}"
  	do
   		  curr_product=$k
    		  curr_pnl=${shortcode_pnl["$k"]}
		  echo "<tr><td>$curr_product</td><td>$curr_pnl</td></tr>" >> $Monthly_Top_Gainer_Loser;
  	done
	cat "/home/hardik/PNLProject/html/pnl_report_footer.txt" >> $Monthly_Top_Gainer_Loser;
}

Get_Year_Top_Gainers_Losers (){
        echo "Get Top Gainer Yearly"
        declare -A shortcode_pnl
        shortcode_pnl=()
        Year_Top_Gainer_Loser=$REPORTS_DEST_DIR$server"/"$yyyy"/""top_gainers_losers.html" ;
       	cat "/home/hardik/PNLProject/html/pnl_report_header.txt" | sed 's/PNL REPORTS/'$server' TOP GAINERS AND LOSERS year ['$yyyy' ] /g' > $Year_Top_Gainer_Loser ;
       	while read line 
        do
        	curr_product=`echo $line | cut -d' ' -f 1`
     		curr_pnl=`echo $line | cut -d' ' -f 2`
     		if [ ${shortcode_pnl[$curr_product]+abc} ] ; then
                        shortcode_pnl[$curr_product]=`echo "${shortcode_pnl[$curr_product]} + $curr_pnl" | bc`
                else
                      	shortcode_pnl[$curr_product]=$curr_pnl
                fi
        done < $Yearly_stratFile
        echo "<th>Product</th><th>PNL</th></tr></thead><tbody>">> $Year_Top_Gainer_Loser ;
      	for k in "${!shortcode_pnl[@]}"
  	    do
   		     curr_product=$k
   		     curr_pnl=${shortcode_pnl["$k"]}
   		     echo "<tr><td>$curr_product</td><td>$curr_pnl</td></tr>" >> $Year_Top_Gainer_Loser;
    	done
   	cat "/home/hardik/PNLProject/html/pnl_report_footer.txt" >> $Year_Top_Gainer_Loser;
}

Compute_Historic_Data () {
  echo "Compute_Historic_Data::";
 	for server in $servers;
  	do
    		cd $SOURCE_DIR$servers
    		echo -e "$PWD\n"
    		for file in `ls -I '*.gz.gz' | grep "log.*"`;
 		    do
 			     echo -e "       file:: $file\n" 
 			     stratNo=$(echo $file| cut -d'.' -f 3);
			     date=$(echo $file| cut -d'.' -f 2);
 			     yyyy=${date:0:4};
 			     mm=${date:4:2};
			     dd=${date:6:2};
			     
 			     Daily_Top_Gainer_Loser_FILE=$REPORTS_DEST_DIR$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo".top_gainers_losers.html" ;
			     echo $REPORTS_DEST_DIR$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo".top_gainers_losers.html" ;
                             if [ $server == "IND14" ] || [ $server == "IND15" ] || [ $server == "IND19" ] || [ $server == "IND20" ] ; then
                               echo "${server}:PROTRESULT:${file}"
                               products=`zless $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                             else
                               echo "${server}:SIMRESULT:${file}"
                               products=`zless $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                             fi

           #      mkdir -p $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd
           #      mkdir -p $REPORTS_DEST_DIR$server"/"$yyyy"/"$mm"/"$dd   
    
           #top 5 gainers
           #      echo $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_gainers$stratNo
           #echo -e " top gainers:: \n"
           #      echo -e "$products\n" | tail -6 > $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_gainers$stratNo

           #top 5 losers
           #echo -e " top losers:: \n"
           #      echo -e "$products\n" | head -5 > $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd$top_losers$stratNo      
           #      Create_Daily_Top_Products_File $DEST_DIR$server"/"$yyyy"/"$mm"/"$dd $stratNo $Daily_Top_Gainer_Loser_FILE $date

 			     Get_Curr_Top_Gainers_Losers $*
		    done
 	done
  echo "DONE Compute_Historic_Data"
}

Compute_Daily_Data() {
  	declare -a stratfiles=();
  	echo "Compute_Daily_Data::";
  	[ $# -gt 1 ] || print_msg_and_exit "Usage : < script > < TYPE(DAILY/HISTORIC) > < DATE >" 
  	date=$2;
  	if [ "$2" == "YESTERDAY" ] ; then
    		date=`date -d "1 day ago" +"%Y%m%d"` ;
  	fi
 	echo -e "date=> $date\n"
  	yyyy=${date:0:4};
 	mm=${date:4:2};
 	dd=${date:6:2};
  	for server in $servers;
  	do
    		cd $SOURCE_DIR$server
    		echo -e "for $server\n"
    		for file in `ls -I '*.gz.gz' | grep "log.$date.*"`;
    		do
      			stratNo=$(echo $file | cut -d'.' -f 3);
      			date_log=$(echo $file | cut -d'.' -f 2);
#                        if [ $server = IND15  -a  $stratNo = 123453 ] || [ $server = IND16  -a  $stratNo = 123706 ] || [ $server = IND17  -a  $stratNo = 123804 ] || [ $server = IND18  -a  $stratNo = 123520 ] || [ $server = IND19  -a  $stratNo = 123496 ] || [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]]; then
#                                 continue;
#                       fi
			if [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]] ; then
                	     continue;
		        fi
      			stratfiles+=("$date_log.$stratNo");
	                Daily_Top_Gainer_Loser_FILE=$REPORTS_DEST_DIR$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo".top_gainers_losers.html" ;
                        if [ $server == "IND14" ] || [ $server == "IND15" ] || [ $server == "IND19" ] || [ $server == "IND20" ] ; then
                          echo "${server}:PROTRESULT:${file}"
                          products=`zless $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                        else
                          echo "${server}:SIMRESULT:${file}"
                          products=`zless $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                        fi
			Get_Curr_Top_Gainers_Losers $*
    		done
  	done
}
Compute_Historic_2_Daily(){
	declare -a day_a=(01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31)
	declare -a month_a=(01 02 03 04 05 06 07 08 09 10 11 12)
	declare -a year_a=(2017 2018 2019)
	for i in "${year_a[@]}"
	do	for j in "${month_a[@]}"
		do
			for k in "${day_a[@]}"
			do 
				declare -a stratfiles=();
				date=$i$j$k;
				yyyy=${date:0:4};
			        mm=${date:4:2};
        			dd=${date:6:2};
   				for server in $servers;
        			do
                			cd $SOURCE_DIR$server
                			echo -e "for $server\n"
                			for file in `ls -I '*.gz.gz' | grep "log.$date.*"`;
                			do
                        			stratNo=$(echo $file | cut -d'.' -f 3);
                        			date_log=$(echo $file | cut -d'.' -f 2);
#                        			if [ $server = IND15  -a  $stratNo = 123453 ] || [ $server = IND16  -a  $stratNo = 123706 ] || [ $server = IND17  -a  $stratNo = 123804 ] || [ $server = IND18  -a  $stratNo = 123520 ] || [ $server = IND19  -a  $stratNo = 123496 ] || [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]]; then
#                                  			continue;
#                        			fi
                                                if [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]] ; then
                		     			continue;
		                		fi

                        			stratfiles+=("$date_log.$stratNo");
                        			Daily_Top_Gainer_Loser_FILE=$REPORTS_DEST_DIR$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo".top_gainers_losers.html" ;
                                                if [ $server == "IND14" ] || [ $server == "IND15" ] || [ $server == "IND19" ] || [ $server == "IND20" ] ; then
                                                  echo "${server}:PROTRESULT:${file}"
                                                  products=`zless $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                                                else
                                                  echo "${server}:SIMRESULT:${file}"
                                                  products=`zless $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2`
                                                fi

                        			Get_Curr_Top_Gainers_Losers $*
                			done
        			done
			done
		done
	done
}

Compute_Month_Data(){
	declare -a stratfiles=();
	echo "Compute_Monthly_Data::";
  	[ $# -gt 1 ] || print_msg_and_exit "Usage : < script > < TYPE(DAILY/HISTORIC) > < DATE >"
  	date=$2;
  	echo -e "date Year and month=> $date\n"
  	yyyy=${date:0:4};
  	mm=${date:4:2};
  	rm -r $Monthly_Dir
  	mkdir $Monthly_Dir
  	for server in $servers;
  	do
    		cd $SOURCE_DIR$server
    		echo -e "for $server\n"
		Monthly_stratFile=$Monthly_Dir"/"$server".MonthlyStratFile."$mm ;
		>$Monthly_stratFile
    		for file in `ls -I '*.gz.gz' | grep "log.$yyyy$mm...*"`;
    		do
        		stratNo=$(echo $file | cut -d'.' -f 3);
			date_log=$(echo $file | cut -d'.' -f 2);
#        		if [ $server = IND15  -a  $stratNo = 123453 ] || [ $server = IND16  -a  $stratNo = 123706 ] || [ $server = IND17  -a  $stratNo = 123804 ] || [ $server = IND18  -a  $stratNo = 123520 ] || [ $server = IND19  -a  $stratNo = 123496 ] || [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]] ; then
#				         continue;
#			fi
                        if [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]] ; then
                	     continue;
		        fi
            		stratfiles+=("$date_log.$stratNo");      
                        if [ $server == "IND14" ] || [ $server == "IND15" ] || [ $server == "IND19" ] || [ $server == "IND20" ] ; then
                          echo "${server}:PROTRESULT:${file}"
                          zless $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2 >>$Monthly_stratFile
                        else
                          echo "${server}:SIMRESULT:${file}"
                          zless $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2 >>$Monthly_stratFile
                        fi
   		 done
		 Get_Month_Top_Gainers_Losers $*
  	done
}

Compute_Year_Data(){
  declare -a stratfiles=();
  echo "Compute_Yearly_Data::";
  [ $# -gt 1 ] || print_msg_and_exit "Usage : < script > < TYPE(DAILY/HISTORIC) > < DATE >"
  date=$2;
  echo -e "Year=> $date\n"
  yyyy=${date:0:4};
  rm -r $Yearly_Dir
  mkdir $Yearly_Dir
  for server in $servers;
  do
    	cd $SOURCE_DIR$server
    	echo -e "for $server\n"
	Yearly_stratFile=$Yearly_Dir"/"$server".YearlyStratFile" ;
	>$Yearly_stratFile
    	for file in `ls -I '*.gz.gz' | grep "log.$yyyy.....*"`;
    	do
     		stratNo=$(echo $file | cut -d'.' -f 3);
		date_log=$(echo $file | cut -d'.' -f 2);
#      		if [ $server = IND15  -a  $stratNo = 123453 ] || [ $server = IND16  -a  $stratNo = 123706 ] || [ $server = IND17  -a  $stratNo = 123804 ] || [ $server = IND18  -a  $stratNo = 123520 ] || [ $server = IND19  -a  $stratNo = 123496 ] || [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]] ; then
#        		continue;
#        	fi
                if [[ " ${stratfiles[*]} " == *"$date_log.$stratNo"* ]] ; then
                     continue;
		fi
        	stratfiles+=("$date_log.$stratNo");
        	echo -e "\tfile, stratNo => $file, $stratNo\n";
                if [ $server == "IND14" ] || [ $server == "IND15" ] || [ $server == "IND19" ] || [ $server == "IND20" ] ; then
                  echo "${server}:PROTRESULT:${file}"
                  zless $file | grep PORTRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2 >>$Yearly_stratFile
                else
                  echo "${server}:SIMRESULT:${file}"
                  zless $file | grep SIMRESULT | awk '{print $3,$5}' | tac | awk '!seen[$1]++' | sort -n -k2 >>$Yearly_stratFile
                fi
    	done
        Get_Year_Top_Gainers_Losers $*
  done
}

init () {
  echo "number of arg:: $# ";
  [ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < TYPE(DAILY/HISTORIC) > < DATE >"
  OPTION=$1;
  case $OPTION in
    DAILY)
      Compute_Daily_Data $*
      ;;
    MONTHLY)
      Compute_Month_Data $*
      ;;
    YEARLY)
      Compute_Year_Data $*
      ;;
    HISTORIC)
      Compute_Historic_Data $*
      ;;
    H2)
      Compute_Historic_2_Daily $*
      ;;
    *)
      print_msg_and_exit "Usage : < script > < DATE > <TYPE(DAILY/HISTORIC)>";
  esac
}

init $*
