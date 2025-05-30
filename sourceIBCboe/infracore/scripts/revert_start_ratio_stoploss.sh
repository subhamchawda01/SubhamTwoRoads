
GetStopLossValue() {
	declare -iA stoplossvalue_to_count
	for symbolconfig in `cat ${live_file} | grep "_MM" | awk -F "=" '{print $2}'`;
	do
		stoploss=`grep -w "STOP_LOSS" $symbolconfig'/MainConfig.cfg' | awk -F "=" '{print $2}' | awk '{print $1}'`
                stoplossvalue_to_count[$stoploss]+=1
	done
	max=0
	for key in "${!stoplossvalue_to_count[@]}";
	do
		[ ${stoplossvalue_to_count[${key}]} -gt ${max} ] &&  max=${key} ;
	done
	[ ${max} -ne 0 ] && new_stop_loss_val=${max}
}

today=$1
echo "Inside Revert"
mail_file=/tmp/stoploss_revert_mail_${today}
>${mail_file}
start_ratio_dir=`cat /home/dvctrader/ATHENA/run.sh | grep START_RATIO \
		| grep -v "#" | awk '{print $2}' | awk  -F "/" '{print $1"/"$2"/"$3"/"$4"/"$5}'`

stoploss_file=${start_ratio_dir}'/old_stoploss.'${today}

cd ${start_ratio_dir}

[ ! -f ${stoploss_file} ] && exit;

new_stop_loss_val=15000
echo "func call"
#GetStopLossValue
echo "func end"

echo "file ${stoploss_file}"

while read -r line;
do 
	prod=`echo $line | awk '{print $1}'`
	sed -i "s/\<STOP_LOSS = .*\>/STOP_LOSS = ${new_stop_loss_val}/g" "NSE_${prod}_MM/MainConfig.cfg"
done < ${stoploss_file}

for prod in `cat ${stoploss_file} | awk '{print $1}'`;
do 
  new_value=`grep -w STOP_LOSS "NSE_${prod}_MM/MainConfig.cfg" | awk '{print $3}'`
  echo "$prod" $new_value >> ${mail_file}
done

rm -rf ${start_ratio_dir}'/old_stoploss.'${today}

