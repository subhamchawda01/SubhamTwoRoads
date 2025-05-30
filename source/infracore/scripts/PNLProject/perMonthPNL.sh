#!/bin/bash

servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20" ;
#SourceDir="/home/hardik/PNLProject/"
#CURDIR="tradelogs/"
path="/home/hardik/PNLProject/PNLCount1/";

#/bin/bash temp.sh
for server in $servers;
  do
        cd $path$server
#        echo -e "$PWD\n"
 #       echo -e "for $server\n"
        for yyyy in `ls -I '*.*'`;
          do	
		declare -A yy_stratID_open
		declare -A yy_stratID_pnl
		yy_stratID_open=()
		yy_stratID_pnl=()
		cd $path$server"/"$yyyy
		for mm in `ls -I '*.*'`;
		  do
			#echo -e "	yearmm:: $yyyy $mm\n"
			cd $path$server"/"$yyyy"/"$mm
			#echo -e "    inside:: $PWD\n"
			totalPNL=0
			totalNumOpen=0
			for dd in `ls -d */`;
			  do
				#dailyPNL=0;
				#dailyNumOpen=0;
				cd $path$server"/"$yyyy"/"$mm"/"$dd
				#echo -e "    finally:: $PWD, for  date $dd\n";
				for stratID in `ls -I '*.*'`;
				  do
					#echo -e  "cat:: $stratID\n"
					stratPNL=`cat $stratID | cut -d' ' -f1`;
					stratNumOpen=`cat $stratID | cut -d' ' -f2`;
					[ -z $stratPNL ] && stratPNL=0
					[ -z $stratNumOpen ] && stratNumOpen=0
					totalPNL=$((totalPNL + stratPNL)) ;
					totalNumOpen=$((totalNumOpen + stratNumOpen)) ;
					#echo -e "stratID, NUMopen, totalpnl, totalNumopen:: $stratPNL, $stratNumOpen, $totalPNL, $totalNumOpen\n"
					echo  "$stratID $stratPNL $stratNumOpen" >> $path$server"/"$yyyy"/"$mm"/"$dd"dailyStratPNL."$yyyy$mm${dd:0:2};
					echo  "$yyyy$mm${dd:0:2} $stratPNL $stratNumOpen" >> $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID;
					#echo -e "hello\n"
				  done
#				cd ../
			#	echo -e "second hello \n"
			  done
			#`sort $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID | uniq > "/tmp/temp"`; 
			#`mv "/tmp/temp" $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID`;
			echo -e $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID;
			cd $path$server"/"$yyyy"/"$mm ;
			#echo -e "PWD-> $PWD\n"	;
			file=`ls | grep 'monthlyStratPNL.*'`;
#			echo -e "file name= $file\n"
			> "TotalMonthlyStratPNL."$yyyy$mm ;
			for monthlyStratPNL in `ls | grep 'monthlyStratPNL.*'`;
			  do
#				echo -e "monthlystrat:: $monthlyStratPNL\n"
				#stratID= $(echo $monthlyStratPNL | cut -d'.' -f2)
				stratID=`echo $monthlyStratPNL | awk -F "." '{print $2}'`	
				#echo -e "    monthly dir:: $PWD\n"
				totalPNL=`cat $monthlyStratPNL | awk '{t+=$2} END {print t}'`
				totalNumOpen=`cat $monthlyStratPNL | awk '{t+=$3} END {print t}'`
#				echo -e "StratID, PNL, NumOpen:: $stratID,\n $totalPNL,\n $totalNumOpen \n"
                                echo -e "$stratID $totalPNL $totalNumOpen" >> "TotalMonthlyStratPNL."$yyyy$mm ;
				if [ ${yy_stratID_open[$stratID]+abc} ] ; then
					((yy_stratID_pnl[$stratID]+=$totalPNL))
					((yy_stratID_open[$stratID]+=$totalNumOpen))
					echo -e $totalPNL >> "/tmp/resPNL"{$yyyy}".txt"
				else
					yy_stratID_pnl[$stratID]=$totalPNL
                                        yy_stratID_open[$stratID]=$totalNumOpen
					echo -e $totalPNL >> "/tmp/resPNL"{$yyyy}".txt"
				fi
			  done
#			echo -e "endmonth:: $mm, curdir:: $PWD\n"
		  done
	     cd ../
	     total_year_pnl=0
	     total_year_open=0
	     > "YearlyStratPNL."$yyyy
	     for stratID in "${!yy_stratID_pnl[@]}"; do 
		echo -e "$stratID ${yy_stratID_pnl[$stratID]} ${yy_stratID_open[$stratID]}" >> "YearlyStratPNL."$yyyy ; 
	     	((total_year_pnl+=${yy_stratID_pnl[$stratID]}))
		((total_year_open+=${yy_stratID_open[$stratID]}))
	     done
	     echo -e "$total_year_pnl $total_year_open" > "TotalYearlyStratPNL."$yyyy ;
	  done
  done
