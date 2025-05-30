#!/bin/bash

print_msg_and_exit (){
 echo $1;
 exit
}

servers="IND15 IND16 IND17 IND18 IND19 IND20" ;
SourceDir="/home/hardik/PNLProject/copy/"
DestPath="/home/hardik/PNLProject/PNLCount1/";
path="/home/hardik/PNLProject/PNLCount1/";

file_list="/home/hardik/PNLProject/file_list.txt"

[ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < DATE >" ;

  date=$1;

  if [ "$1" == "YESTERDAY" ] ; then
    date=`date -d "1 day ago" +"%Y%m%d"` ;
  fi

echo -e "date=> $date\n"
yyyy=${date:0:4};
mm=${date:4:2};
dd=${date:6:2};

declare -a stratfiles=();

 for server in $servers;
  do
        cd $SourceDir$server
	echo -e "$PWD\n"
	echo -e "for $server\n"
        for file in `ls -I '*.gz.gz' | grep "log.$date.*"`;
          do
		stratNo=$(echo $file| cut -d'.' -f 3);
   		echo -e "\tfile, stratNo => $file, $stratNo\n";
		#if [ $server = IND15  -a  $stratNo = 123453 ] || [ $server = IND16  -a  $stratNo = 123706 ] || [ $server = IND17  -a  $stratNo = 123804 ] || [ $server = IND18  -a  $stratNo = 123520 ] || [ $server = IND19  -a  $stratNo = 123496 ] ; then
                #                echo "Inside IF file:"$file"  Server"$server
                #                continue;
                #        fi
		if ! `grep -Fxq "$date.$stratNo" $file_list`;
		then
		    echo -e "$date.$stratNo not present\n" 	
#		else
	#	    echo -e "$date.$stratNo already present\n" ;
		    echo "$date.$stratNo" >> $file_list
		    stratfiles+=("$date.$stratNo");

		    mkdir -p "$DestPath$server/$yyyy/$mm/$dd"
		     echo -e "before PNL\n"
                     totalPNL=`zless -f $file | grep PORTFOLIO | awk 'END {print $3}'`
                     totalNumOpen=`zless -f $file | grep PORTFOLIO | awk 'END {print $11}'`
		     echo -e "StratNO Pnl, Numopen::$stratNo $totalPNL, $totalNumOpen\n"
                     echo "$totalPNL $totalNumOpen" > $DestPath$server"/"$yyyy"/"$mm"/"$dd"/"$stratNo;
		fi	
          done
  done

echo -e "stratfiles=> ${stratfiles[@]}\n"

 for server in $servers;
 do
   # mkdir -p
    [ -d "$DestPath$server/$yyyy/$mm/$dd" ] || continue 
    cd $DestPath$server"/"$yyyy"/"$mm"/"$dd
    echo -e "$PWD\n"
    echo -e "for $server\n"
    for stratID in `ls -I '*.*'`;
    do
        echo -e "****stratNo=> $stratID\n"
	if [[ " ${stratfiles[*]} " == *"$date.$stratID"* ]]; then
           TotalMonthlystratfile=$path$server"/"$yyyy"/"$mm"/TotalMonthlyStratPNL."$yyyy$mm
           Monthlystratfile=$path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID
           echo -e  "cat:: $stratID\n"
              stratPNL=`cat $stratID | cut -d' ' -f1`
              stratNumOpen=`cat $stratID | cut -d' ' -f2`
	      [ -z $stratPNL ] && stratPNL=0
              [ -z $stratNumOpen ] && stratNumOpen=0
              echo $stratPNL $stratNumOpen
	   echo -e "daily=> stratID, NUMopen, totalpnl, totalNumopen:: $stratPNL, $stratNumOpen, $totalPNL, $totalNumOpen\n\n";
      	   echo "$stratID $stratPNL $stratNumOpen" >> $path$server"/"$yyyy"/"$mm"/"$dd"/dailyStratPNL."$yyyy$mm$dd;
           if `grep -q $stratID $Monthlystratfile`;
           then
              lineToReplace=`grep $yyyy$mm$dd $Monthlystratfile | head -1`
              replacement=$yyyy$mm$dd" "$stratPNL" "$stratNumOpen
              sed -i "s/${lineToReplace}/${replacement}/" $Monthlystratfile
           else
              echo "$yyyy$mm$dd $stratPNL $stratNumOpen" >> $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID;
 	   fi

 	   `sort $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID | uniq > "/tmp/temp"; mv "/tmp/temp" $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID`
      #echo -e "hello\n"

      	   totalPNL=`cat $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID | awk '{t+=$2} END {print t}'`
      	   totalNumOpen=`cat $path$server"/"$yyyy"/"$mm"/monthlyStratPNL."$stratID | awk '{t+=$3} END {print t}'`
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
	    cd $DestPath$server"/"$yyyy
	    echo "YEar data"
	    for mm in `ls -I '*.*'`;
	    do 
	    	cd $DestPath$server"/"$yyyy"/"$mm
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
		done < $path$server"/"$yyyy"/"$mm"/TotalMonthlyStratPNL."$yyyy$mm
	    done
	    Yearlystratfile=$path$server"/"$yyyy"/YearlyStratPNL."$yyyy
            TotalYearlystratfile=$path$server"/"$yyyy"/TotalYearlyStratPNL."$yyyy 
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
