cd /spare/local/FUT_BarData/
date_="$(date +'%Y%m%d')"

for i in *; do 
   /home/dvctrader/nishit/execs/backadjustment_exec --start_date 20051101 --end_date $date_ --index_file /home/dvctrader/nishit/execs/Complete_Index_No_Header.csv --output_file /spare/local/FUT_BarData_Adjusted_Temp/$i --ticker $i
done

#while IFS='' read -r line || [[ -n "$line" ]]; do ratio=$(grep 20190529 "/spare/local/NseHftFiles/Ratio/EndRatio/NSE_"$line"_FUT1_NSE_"$line"_FUT0" | awk '{print $2}');echo $ratio; if [[ "$ratio" != "" ]]; then less /spare/local/BarData/$line | awk '{print $1,"\011",$2,"\011",$3,"\011",$4,"\011",$5,"\011",$6*'$ratio',$7*'$ratio',$8*'$ratio',$9*'$ratio',$10/'$ratio',$11,"\011",$12*'$ratio'}' > temp; mv temp "/spare/local/BarData/"$line;fi; done < /home/dvctrader/RESULTS_FRAMEWORK/strats/prod_file_extended
