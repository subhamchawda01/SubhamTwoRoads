#!/bin/bash
SUB="Data_unavailable_on_"`hostname`

while [ true ]
do
	ps -ef | grep tradeinit | grep -v grep |awk '{print $(NF)}' > /spare/local/files/running_queries_list
	for i in `cat /spare/local/files/running_queries_list` 
	 do logfile="/spare/local/logs/tradelogs/log."`date +%Y%m%d`"."$i ;
	  #cmd="grep -nr STRATEGYLINE" $logfile "| awk -F: '{print \$2}' | tail -n1 | awk '{print \$4}'"
	  modelfile=`grep -nr STRATEGYLINE $logfile | awk -F: '{print $2}' | tail -n1 | awk '{print $4}'`
          shortcode=`grep -nr STRATEGYLINE $logfile | awk -F: '{print $2}' | tail -n1 | awk '{print $2}'`
	  x=`grep -nr "All Indicators Ready\|No Indicators\! So by default Ready\!" $logfile | awk -F: '{print $1}' | tail -n1`
	  #echo $x
	  if [ "$x" == "" ]
	   then x=0;
	  fi
	  y=`grep -nr STRATEGYLINE $logfile | awk -F: '{print $1}' | tail -n1`
	  #echo $y
	  if [ "$y" == "" ]
	   then y=0;
	  fi
	  z=`grep -nr "MarketUpdateManagerE:start" $logfile | awk -F: '{print $1}' | tail -n1`
	  #echo $z
	  if [ "$z" == "" ]
	   then z=0;
	  fi
	  #echo $x $y $z
	  if [ $x -lt $y ] 
	   then
	   #echo "1 "$logfile" 2 "$modelfile" 3 "$shortcode
	   count=0;
	   for line in `grep  -w INDICATOR $modelfile | sed 's/ /_~_/g'`
	    do
		ind_list[ $count ]=$line
		#echo ${ind_list[ $count ]}" "$count" "$line
		count=`expr $count + 1`
	    done
	   count=`expr $count - 1`
           echo "Indicators not ready for Query id "$i" for product "$shortcode > /spare/local/files/mailbody_kkp
	   if [ $y -gt $z ]
		then 
		for j in $(seq 0 $count); do
		  echo ${ind_list[ $j ]} | sed 's/_~_/ /g' >> /spare/local/files/mailbody_kkp
		  #echo ${ind_list[ $j ]} "temp "$j | sed 's/_~_/ /g'
		 # echo "\n" >> /spare/local/files/mailbody_kkp
		done
	   else
		non_ready_index_list=`grep "Indicator Not Ready" $logfile | tail -n50 | cut -d " " -f 2- | sort| uniq | awk '{print $6}'`
		for j in `grep "Indicator Not Ready" $logfile | tail -n50 | cut -d " " -f 2- | sort| uniq | awk '{print $6}'`; do
		  echo ${ind_list[ $j ]} | sed 's/_~_/ /g' >> /spare/local/files/mailbody_kkp
		  #echo ${ind_list[ $j ]}"temp2 "$j | sed 's/_~_/ /g' 
		 # echo "\n" >> /spare/local/files/mailbody_kkp
		done
	   fi
     if [ "$i" != "252021" ]
      then
	    /bin/mail -s $SUB -r "nseall@tworoads.co.in" "nseall@tworoads.co.in"  < /spare/local/files/mailbody_kkp
	    #/bin/mail -s $SUB -r "kishenp@tworoads.co.in" "kishenp@tworoads.co.in"  < /spare/local/files/mailbody_kkp
     fi
	  fi 
	 done
        sleep 300
done
