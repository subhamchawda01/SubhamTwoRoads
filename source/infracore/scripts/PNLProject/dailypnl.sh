#!/bin/bash/

servers="IND15 IND16 IND17" ;
SourceDir="/home/hardik/PNLProject/copy/"
#SourceDir="/home/hardik/PNLProject/copy/"
CURDIR="tradelogs/"
DestPath="/home/hardik/PNLProject/PNLCount1/";

file_list="/home/hardik/PNLProject/file_list.txt"
date=$1;
yyyy=${date:0:4};
mm=${date:4:2};
dd=${date:6:2};
 for server in $servers;
  do
        cd $SourceDir$server
	#echo -e "$PWD\n"
	echo -e "for $server\n"
        for file in `ls -I '*.gz.gz' | grep "log.$date.*.gz"`;
          do
		stratNo=$(echo $file| cut -d'.' -f 3);
		
		if ! `grep -Fxq "$date.$stratNo" $file_list`;
		then
			echo -e "$date.$strat Not present\n" 	
#		else
	#	    echo -e "$date.$stratNo already present\n" ;
		    echo "$date.$stratNo" >> $file_list

		   if [ ! -d "$DestPath$server/$yyyy/$mm/$dd" ]; then
		#	echo -e "$DestPath$server/$yyyy/$mm/$dd not present\n";
			mkdir -p "$DestPath$server/$yyyy/$mm/$dd"
		   fi
		
                     totalPNL=`less $file | grep PORTFOLIO | awk '{t+=$3} END {print t}'`
                     totalNumOpen=`less $file | grep PORTFOLIO | awk '{t+=$11} END {print t}'`
		     echo -e "pnl, Numopen::$stratNo $totalPNL, $totalNumOpen\n"
                     echo "$totalPNL $totalNumOpen" >> $DestPath$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo;
		fi	
          done
  done
