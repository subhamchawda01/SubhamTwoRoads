#!/bin/bash
date_=$1
prod=$2
expiry=$3
fut1_expiry=$4
fut2_expiry=$5
echo $1,$2,$3,$4,$5
start_=$(date -d "$date_ 0345" +%s)
end_=$(date -d "$date_ 1000" +%s)
product_=NSE_"$prod"_FUT0
fut1_product_=NSE_"$prod"_FUT1
fut2_product_=NSE_"$prod"_FUT2
shc_="$prod"_FF_0_0
fut1_shc_="$prod"_FF_0_1
fut2_shc_="$prod"_FF_0_2

lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' /spare/local/FUT_BarData/$prod) 
#echo $lineNum
if [ -z "$lineNum" ]
then
	echo "no corrupt data for $prod"
else
	sed -i "$lineNum,\$d" "/spare/local/FUT_BarData/$prod"
fi

data=$(/home/dvctrader/usarraf/cvquant_install/basetrade/bin/mkt_trade_logger SIM "$product_" "$date_" | grep OnTradePrint)
fut1_data=$(/home/dvctrader/usarraf/cvquant_install/basetrade/bin/mkt_trade_logger SIM "$fut1_product_" "$date_" | grep OnTradePrint)
fut2_data=$(/home/dvctrader/usarraf/cvquant_install/basetrade/bin/mkt_trade_logger SIM "$fut2_product_" "$date_" | grep OnTradePrint)
# echo $fut2_data
low_="1000000"
high_="0"
open_="0"
close_="0"
open_time_="0"
close_time_="0"
volume_="0"
count_="0"
while IFS= read -r line
do
   time_=$(echo "$line" | awk '{print $1}')
   vol_=$(echo "$line" | awk '{print $5}')
   px_=$(echo "$line" | awk '{print $7}')
   minute_=`expr $start_ + 60`
   #echo $px_
   while (( $(echo "$minute_ < $time_" | bc -l) ));
   #while [ "$minute_" -lt "$time_" ]
   do
   		if(( $(echo "$open_ != 0" | bc -l) )); 
		then
   			echo -e "$start_"\\t"$shc_"\\t"$open_time_"\\t"$close_time_"\\t"$expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_" >> "/spare/local/FUT_BarData/temp$prod"
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
	echo -e "$start_"\\t"$shc_"\\t"$open_time_"\\t"$close_time_"\\t"$expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_" >> "/spare/local/FUT_BarData/temp$prod"
fi


low_="1000000"
high_="0"
open_="0"
close_="0"
open_time_="0"
close_time_="0"
volume_="0"
count_="0"
start_=$(date -d "$date_ 0345" +%s)
end_=$(date -d "$date_ 1000" +%s)
while IFS= read -r line
do
   time_=$(echo "$line" | awk '{print $1}')
   vol_=$(echo "$line" | awk '{print $5}')
   px_=$(echo "$line" | awk '{print $7}')
   minute_=`expr $start_ + 60`
   #echo $px_
   while (( $(echo "$minute_ < $time_" | bc -l) ));
   #while [ "$minute_" -lt "$time_" ]
   do
         if(( $(echo "$open_ != 0" | bc -l) )); 
      then
            echo -e "$start_"\\t"$fut1_shc_"\\t"$open_time_"\\t"$close_time_"\\t"$fut1_expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_" >> "/spare/local/FUT_BarData/temp$prod"
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

done < <(printf '%s\n' "$fut1_data")
if(( $(echo "$open_ != 0" | bc -l) )); 
then
   echo -e "$start_"\\t"$fut1_shc_"\\t"$open_time_"\\t"$close_time_"\\t"$fut1_expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_" >> "/spare/local/FUT_BarData/temp$prod"
fi

echo "all smooth"

low_="1000000"
high_="0"
open_="0"
close_="0"
open_time_="0"
close_time_="0"
volume_="0"
count_="0"
start_=$(date -d "$date_ 0345" +%s)
end_=$(date -d "$date_ 1000" +%s)
if [[ -z "$fut2_data" ]]; then
      echo "hello"

else
   while IFS= read -r line
   do


      time_=$(echo "$line" | awk '{print $1}')
      vol_=$(echo "$line" | awk '{print $5}')
      px_=$(echo "$line" | awk '{print $7}')
      minute_=`expr $start_ + 60`
      #echo $px_
      while (( $(echo "$minute_ < $time_" | bc -l) ));
      #while [ "$minute_" -lt "$time_" ]
      do
            if(( $(echo "$open_ != 0" | bc -l) )); 
         then
               echo -e "$start_"\\t"$fut2_shc_"\\t"$open_time_"\\t"$close_time_"\\t"$fut2_expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_" >> "/spare/local/FUT_BarData/temp$prod"
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

   done < <(printf '%s\n' "$fut2_data")
   if(( $(echo "$open_ != 0" | bc -l) )); 
   then
      echo -e "$start_"\\t"$fut2_shc_"\\t"$open_time_"\\t"$close_time_"\\t"$fut2_expiry"\\t"$open_"\\t"$close_"\\t"$low_"\\t"$high_"\\t"$volume_"\\t"$count_" >> "/spare/local/FUT_BarData/temp$prod"
   fi

fi

sort /spare/local/FUT_BarData/temp$prod >> "/spare/local/FUT_BarData/$prod"
rm /spare/local/FUT_BarData/temp$prod
echo $prod "done"