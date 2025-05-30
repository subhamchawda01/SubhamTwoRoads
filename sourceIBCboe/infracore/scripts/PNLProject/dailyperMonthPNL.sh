#!/bin/bash/

servers="IND11 IND12 IND13" ;

#SourceDir="/home/hardik/PNLProject/"
#CURDIR="tradelogs/"
path="/home/hardik/PNLProject/PNLCount1/";

#/bin/bash temp.sh

file_list="/home/hardik/PNLProject/file_list.txt"
date=$1;
    yyyy=${date:0:4};
    mm=${date:4:2};
    dd=${date:6:2};

for server in $servers;
do
  #for date in $Mon $Tue $Wed $Thurs $Fri
  #do
    [ -d $path$server"/"$yyyy"/"$mm"/"$dd ] || continue 
    cd $path$server"/"$yyyy"/"$mm"/"$dd
    echo -e "$PWD\n"
    echo -e "for $server\n"
    for stratID in `ls -I '*.*'`;
    do  
      if ! `grep -Fxq "$date.$stratID" $file_list`;
      then
	echo -e "	for $server:: $date.$stratID is not present\n"
	stratfile=$path$server"/"$yyyy"/"$mm"/TotalMonthlyStratPNL."$yyyy$mm
           echo -e  "cat:: $stratID\n"
      stratPNL=`cat $stratID | awk '{t+=$1} END {print t}'`
      stratNumOpen=`cat $stratID | awk '{t+=$2} END {print t}'`
                    #echo -e "stratID, NUMopen, totalpnl, totalNumopen:: $stratPNL, $stratNumOpen, $totalPNL, $totalNumOpen\n"
      echo -e "$stratID $stratPNL $stratNumOpen" >> $path$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd;
      echo -e "$yyyy$mm$dd $stratPNL $stratNumOpen" >> $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID;
      #echo -e "hello\n"
      
      totalPNL=`cat $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID | awk '{t+=$2} END {print t}'`
      totalNumOpen=`cat $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID | awk '{t+=$3} END {print t}'`
      echo -e "StratID, PNL, NumOpen:: $stratID,\n $totalPNL,\n $totalNumOpen \n"
      
       if `grep -q $stratID $stratfile`;
       then
           lineToReplace=`grep $stratID $stratfile | head -1`
           replacement=$stratID" "$totalPNL" "$totalNumOpen
	   sed -i "s/${lineToReplace}/${replacement}/" $stratfile
       else
	   echo -e "$stratID $totalPNL $totalNumOpen" >> $stratfile ; 	   
       fi
      fi
   # done
  done
done
