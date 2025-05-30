#!/bin/bash
#!/bin/tcsh

print_msg_and_exit() {
  echo $* ;
  exit ;
}

[ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < Date_before_expiry >"

#day_before_expirydate_="20210929"
day_before_expirydate_=$1
echo "DATE: $day_before_expirydate_"

while IFS='' read -r line || [[ -n "$line" ]]; 
do
  ratio=$(grep $day_before_expirydate_ "/spare/local/NseHftFiles/Ratio/EndRatio/NSE_"$line"_FUT1_NSE_"$line"_FUT0" | awk '{print $2}');
  if [[ "$ratio" != "" ]]; then
    echo "$line - $ratio enter"
    less /spare/local/BarData/$line | awk '{print $1,"\011",$2,"\011",$3,"\011",$4,"\011",$5,"\011",$6*'$ratio',$7*'$ratio',$8*'$ratio',$9*'$ratio',$10/'$ratio',$11,"\011",$12*'$ratio'}' > temp;
    mv temp "/spare/local/BarData/"$line;
    chown dvctrader:infra /spare/local/BarData/$line
  else
    echo "$line - $ratio - RATIO 0"
  fi;
#done < /tmp/prod_ #/home/dvctrader/RESULTS_FRAMEWORK/strats/prod_file_extended #/tmp/FINNIFT #/home/dvctrader/hardik/On_expiry/sc.txt
#done < /home/dvctrader/hardik/On_expiry/bn_sc.txt
done < /home/dvctrader/RESULTS_FRAMEWORK/strats/prod_file_extended #/home/dvctrader/hardik/On_expiry/sc_new.txt 

