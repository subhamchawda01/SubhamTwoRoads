#!/bin/bash

servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20" ;
#servers="IND19" ;
#SourceDir="/home/hardik/PNLProject/"
SourceDir="/home/hardik/PNLProject/copy/"
DestPath="/home/hardik/PNLProject/PNLCount1/";

totalStratPNLfile=".StratPNL.";
totalPNLfile=".totalPNL.";

#declare -a logfiles=();
file_list="/home/hardik/PNLProject/file_list.txt"
true>$file_list
rm -rf ${DestPath}IND*
echo "start"
 for server in $servers;
  do
        cd $SourceDir$server
	echo -e "$PWD\n"
#	echo -e "for $server\n"
        #for file in `ls log.*.gz`;
        for file in `ls -I '*.gz.gz' | grep "log.*"`;
          do		
		echo -e "	file:: $file\n"	
		stratNo=$(echo $file| cut -d'.' -f 3);
		date=$(echo $file| cut -d'.' -f 2);
#		if [ $server = IND15  -a  $stratNo = 123453 ] || [ $server = IND16  -a  $stratNo = 123706 ] || [ $server = IND17  -a  $stratNo = 123804 ] || [ $server = IND18  -a  $stratNo = 123520 ] || [ $server = IND19  -a  $stratNo = 123496 ]; then
#                                echo "Inside IF file:"$file"  Server"$server
#                                continue;
#                        fi
		if ! `grep -Fxq "$date.$stratNo" $file_list`;
		then
	#		echo -e "$file Present\n"
         #       else
		    yyyy=${date:0:4};
		    mm=${date:4:2};
	            dd=${date:6:2};
#		    echo -e "	$date.$stratNo not present\n" ;
		#else
		    echo "$date.$stratNo" >> $file_list
		     #logfiles+=("$date.$stratNo");

              	    mkdir -p "$DestPath$server/$yyyy/$mm/$dd"
		
                    totalPNL=`less $file | grep PORTFOLIO | awk 'END {print $3}'`
                    totalNumOpen=`less $file | grep PORTFOLIO | awk 'END {print $11}'`
#		    echo -e "pnl, Numopen::$stratNo $totalPNL, $totalNumOpen\n"
                    true > $DestPath$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo;
                    echo -e "$totalPNL $totalNumOpen" > $DestPath$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo;
		fi	
          done
  done
#echo ${logfiles[@]};

