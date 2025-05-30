#!/bin/bash
#!/bin/tcsh
date_=$1
prod_file=$2
workers_=$3
type_=$4
expiry_file="/spare/local/expiry_dates"
expiry_date_="0"
fut1_expiry_date_="0"
fut2_expiry_date_="0"
while IFS= read -r var
do
  if [[ "$var" -ge "$date_" ]]; then
  	expiry_date_=$var
  	break
  fi
done < "$expiry_file"


while IFS= read -r var
do
  if [[ "$var" -gt "$expiry_date_" ]]; then
  	fut1_expiry_date_=$var
  	break
  fi
done < "$expiry_file"


while IFS= read -r var
do
  if [[ "$var" -gt "$fut1_expiry_date_" ]]; then
  	fut2_expiry_date_=$var
  	break
  fi
done < "$expiry_file"

echo $expiry_date_ $fut1_expiry_date_ $fut2_expiry_date_
# echo $expiry_date_

commands_=""
non_commands_=""
while IFS= read -r var
do
 #commands_+= echo -e "bash /home/dvctrader/stable_exec/scripts/fut_data_gen.sh $date_ $var $expiry_date_" 
 #xargs -P 4 bash /home/dvctrader/stable_exec/scripts/fut_data_gen.sh $date_ $var $expiry_date_ &
 commands_+=$"$date_ $var $expiry_date_ $fut1_expiry_date_ $fut2_expiry_date_ "'\n'
 non_commands_+=$"$date_ $var $expiry_date_"'\n'
done < "$prod_file"
echo $type_
if [ "$type_" == "ALL" ]; then 
	echo -e "$commands_" | xargs -P $workers_ -n 5 bash /home/dvctrader/stable_exec/scripts/fut_all_data_gen.sh
else
	echo -e "$non_commands_" | xargs -P $workers_ -n 3 bash /home/dvctrader/stable_exec/scripts/fut_data_gen.sh
fi
