dt=$1; 
prog_id=$2;
start_time=$3;
end_time=$4;
tag=$5;
live_file_path=$6;
/home/dvctrader/stable_exec/trade_engine $live_file_path $dt $start_time $end_time $prog_id ADD_DBG_CODE RETAIL_INFO 1>/spare/local/logs/log_${dt}_${tag}  2>/spare/local/logs/log_${dt}_${tag}  #2>/dev/null 1>/dev/null
for prod in `grep CurrentRatio /spare/local/logs/tradelogs/log.${dt}.${prog_id} |  awk '{print $2;}'  | sort | uniq` ; do num=`grep $prod /spare/local/logs/tradelogs/log.${dt}.${prog_id} | grep CurrentRatio |  wc -l` ; num=`expr $num / 2` ; num=`echo $(($num>0?$num:1))`; val=`grep $prod /spare/local/logs/tradelogs/log.${dt}.${prog_id} | grep CurrentRatio | awk '{print $4;}' | sort -n | head -n"$num" | tail -n1`   ; touch /spare/local/NseHftFiles/Ratio/${tag}/"$prod" || exit  ; echo "$dt $val" | cat - /spare/local/NseHftFiles/Ratio/${tag}/"$prod" > /spare/local/NseHftFiles/Ratio/${tag}/"$prod"_tmp_${dt} ; cat /spare/local/NseHftFiles/Ratio/${tag}/"$prod"_tmp_${dt} | awk '!seen[$1]++' | sort -nrk1 > /spare/local/NseHftFiles/Ratio/${tag}/"$prod" ; rm /spare/local/NseHftFiles/Ratio/${tag}/"$prod"_tmp_${dt} ; done
rm /spare/local/logs/log_${dt}_${tag}
rm /spare/local/logs/tradelogs/log.${dt}.${prog_id}
