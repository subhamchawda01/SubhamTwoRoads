#!/bin/bash
tmp_file_="/tmp/files_from_ind13_mv_ratio"

print_usage_and_exit () {
    echo "$0 ind17/ind23" ;
    exit ;
}

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

YYYYMMDD=`date +%Y%m%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

machine_=$1 ;
date_=`date +%Y%m%d`

ssh dvcinfra@10.23.227.63 "cat /var/www/html/market_data_${machine_}.json  |tr '[' '\n' | cut -d',' -f1,19 | sed 's/\"//g' |sed 's/,/ /g' | tail -n +3 |sort -n -k2" >$tmp_file_


breaker_=`cat $tmp_file_ | wc -l`

echo "Total Count: $breaker_"
breaker_=$(( breaker_ / 10 ))
echo "In One Go: $breaker_"
script="/home/dvctrader/ATHENA/run.sh"
echo "Script: $script"
for pos_file in `cat $script | grep -v "#" | grep CONFIG  | grep -v START_RATIO | awk '{print $2}'   | awk -F "/" '{print $1"/"$2"/"$3"/"$4"/"$5"/PositionLimits.csv"}' | sort | uniq`;
do
	cp $pos_file ${pos_file}_${date_}_bkp
	itr=0
	tot_itr=0
  	echo "Position File: $pos_file"
	for prod in `cat $tmp_file_|cut -d' ' -f1`;
    	do
	 prod=`echo "NSE_$prod"`
         if ! grep -q "${prod}_" $pos_file
        	then
          	echo "Not in Position file,${prod} Not Updating.."
          	continue;
         fi	
	 echo "Closing: $prod"
	 itr=$(( itr + 1 ))
	 prod=${prod//&/\\&}
         prodname=`echo "${prod}_MAXLONGPOS"`;
         sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file;
         prodname=`echo "${prod}_MAXSHORTPOS"`;
         sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file;
         prodname=`echo "${prod}_MAXLONGEXPOSURE"`;
         sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file;
         prodname=`echo "${prod}_MAXSHORTEXPOSURE"`;
	 sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file;
 	 if [[ $itr -eq $breaker_ ]]; then
		echo "Going sleep 1m"
 	 	for i in `cat /home/dvctrader/ATHENA/run.sh | grep -v "#" | grep onload_start | awk '{print $6}'`; do 
			echo "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1 ;"
			/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1 ;
         	done
		echo "Iteration End $tot_itr"
		sleep 48
		itr=1
		tot_itr=$(( tot_itr +1 ))
	 fi
	 if [[ $tot_itr -eq 9 ]]; then
                echo "Breaking...";
		break;
         fi
	done 
	echo "Sleep For 10m..."
	sleep 10m;
	echo "Reverting the Position File"
	cp ${pos_file}_${date_}_bkp $pos_file
done
