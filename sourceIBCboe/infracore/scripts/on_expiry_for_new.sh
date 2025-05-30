#!/bin/bash
#!/bin/tcsh

print_msg_and_exit() {
  echo $* ;
  exit ;
}

[ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < Date_before_expiry >"

day_before_expirydate_=$1
echo "DATE: $day_before_expirydate_"

while IFS='' read -r line || [[ -n "$line" ]]; 
do
  ratio=$(grep $day_before_expirydate_ "/spare/local/NseHftFiles/Ratio/EndRatio/NSE_"$line"_NSE_"$line"_FUT0" | awk '{print $2}');
  if [[ "$ratio" != "" ]]; then
    echo "$line - $ratio enter"
    less /home/dvctrader/trash/CASH_BAR/$line | awk '{print $1,"\011",$2,"\011",$3,"\011",$4,"\011",$5,"\011",$6/'$ratio',$7/'$ratio',$8/'$ratio',$9/'$ratio',$10*'$ratio',$11,"\011",$12}' > /home/dvctrader/trash/temp;
    mv /home/dvctrader/trash/temp "/home/dvctrader/trash/CASH_BAR/"$line;
    chown dvctrader:infra /home/dvctrader/trash/CASH_BAR/$line
  else
    echo "$line - $ratio - RATIO 0"
  fi;
done < /tmp/new_prod #new_prod #/home/dvctrader/hardik/On_expiry/sc.txt
#done < /home/dvctrader/hardik/On_expiry/bn_sc.txt
#done < /home/dvctrader/RESULTS_FRAMEWORK/strats/prod_file_extended

