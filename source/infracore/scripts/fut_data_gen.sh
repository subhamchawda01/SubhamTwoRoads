#!/bin/bash
date_=$1
prod=$2
expiry=$3
echo $1,$2,$3
start_=$(date -d "$date_ 0345" +%s)
end_=$(date -d "$date_ 1000" +%s)

if [[ "$date_" == "$expiry" ]]; then
  product_=NSE_"$prod"_FUT1 ;
else
  product_=NSE_"$prod"_FUT0 ;
fi
echo "Product: $product_"
shc_="$prod"_FF_0_0

lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' /spare/local/BarData/$prod) 
#echo $lineNum
if [ -z "$lineNum" ]
then
	echo "no corrupt data for $prod"
else
	sed -i "$lineNum,\$d" "/spare/local/BarData/$prod"
fi

data=$(/home/dvctrader/stable_exec/mkt_trade_logger_20180918 SIM "$product_" "$date_" | grep OnTradePrint)
#echo $data
low_="1000000"
high_="0"
open_="0"
close_="0"
open_time_="0"
close_time_="0"
volume_="0"
count_="0"
#while IFS= read -r line
while IFS=' ' read -r time_ f2 f3 f4 vol_ f6 px_ line 
do
#   time_=$(echo "$line" | awk '{print $1}')
#   vol_=$(echo "$line" | awk '{print $5}')
#   px_=$(echo "$line" | awk '{print $7}')
   minute_=`expr $start_ + 60`
   #echo $px_
   while (( $(echo "$minute_ < $time_" | bc -l) ));
   #while [ "$minute_" -lt "$time_" ]
   do
   		if(( $(echo "$open_ != 0" | bc -l) )); 
		then
   			echo -e "$start_"\\t"$shc_"\\t"$open_time_"\\t"$close_time_"\\t"$expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_"\\t"1" >> "/spare/local/BarData/$prod"
   			low_="1000000"
			high_="0"
			open_="0"
			close_="0"
			volume_="0"
			count_="0"
		fi
   		start_=`expr $start_ + 60`
   		minute_=`expr $start_ + 60`
   done
   if(( $(echo "$open_ == 0" | bc -l) )); 
   #if [ "$open_" -eq "0" ]
   then
   		open_=$px_
   		close_=$px_
   		low_=$px_
   		high_=$px_
   		open_time_=${time_%.*}
   		close_time_=${time_%.*}
   		volume_=$vol_
   		count_="1"
   else
   		close_=$px_
   		close_time_=${time_%.*}
   		volume_=`expr $volume_ + $vol_`
   		count_=`expr $count_ + 1`
   		if(( $(echo "$low_ > $px_" | bc -l) )); 
   		then
   			low_=$px_
   		fi
   		if(( $(echo "$high_ < $px_" | bc -l) )); 
   		then
   			high_=$px_
   		fi
   fi

done < <(printf '%s\n' "$data")
if(( $(echo "$open_ != 0" | bc -l) )); 
then
	echo -e "$start_"\\t"$shc_"\\t"$open_time_"\\t"$close_time_"\\t"$expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_"\\t"1" >> "/spare/local/BarData/$prod"
fi
echo $prod "done"
