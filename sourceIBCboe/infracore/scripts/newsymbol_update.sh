#!/bin/bash
ftp_dir="/spare/local/files/NSEFTPFiles/"
tmp_file1="/tmp/name_update_contract"
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
expiry_days="/tmp/fil_exp_dates"
>$expiry_days
declare -A date_map

GetNearestExpiry() {
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${next_working_day}
  cat ${contract_file} | grep IDXFUT | grep -w NIFTY | awk -v date=$date_ '{if($NF>=date)print $NF}' | sort | uniq >/tmp/expiry_dates_tmp
}

GetNextWorkingDay(){ 
  next_working_day=`/home/pengine/prod/live_execs/update_date $date_ N A`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  while [ $is_holiday = "1" ] 
  do
      next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
       is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  done
  echo  "NextDay: $next_working_day";
}

gen_date_map(){
  while IFS= read -r line
  do
      MSTR=$(echo $(date -d $line +%b) | awk '{print toupper($1)}');
      date_map[$MSTR]=$line
  done </tmp/expiry_dates_tmp
}


if [[ $# != 2 ]];then
  echo "USAGE $0 Date Symbol"
  exit
fi
YYYYMMDD=$1
date_=$1
symbol=$2
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4}
YY=${YYYYMMDD:2:2}
GetNextWorkingDay;
GetNearestExpiry;
gen_date_map;
contract_file="$ftp_dir/$YYYY/$MM/$DD/contract"
ref_file="/spare/local/tradeinfo/NSE_Files/RefData/nse_fo_${next_working_day}_contracts.txt"
echo "Check and update in ${tradeinfo_dir}/ContractFiles/nse_contracts.${next_working_day}"
for exp_d in "${!date_map[@]}";
do 
      grep $symbol[0-9] $contract_file | grep FUT | grep $exp_d >$tmp_file1 
      price=`cat $tmp_file1 | cut -d'|' -f68`
      lotsize=`cat $tmp_file1 | cut -d'|' -f32`
      price=`perl -e "print sprintf(\"%.2f\", $price/100);"`
      echo -e "STKFUT\t$symbol\t$price\t$lotsize\t0.05\t${date_map[$exp_d]}"
done

for exp_d in "${!date_map[@]}";
do
      grep $symbol[0-9] $contract_file | grep FUT | grep $exp_d >$tmp_file1
      price=`cat $tmp_file1 | cut -d'|' -f68`
      lotsize=`cat $tmp_file1 | cut -d'|' -f32`
      price=`perl -e "print sprintf(\"%.2f\", $price/100);"`
      tick1=`grep ${symbol}[0-9] $ref_file | grep OPTSTK|grep $exp_d|grep PE|tail -1| cut -d' ' -f8| awk -v exp_=$exp_d 'BEGIN{ FS=exp_; } {print $2}'`
      tick1=${tick1::-2}
      tick2=`grep ${symbol}[0-9] $ref_file | grep OPTSTK|grep $exp_d|grep PE|tail -2|head -1| cut -d' ' -f8| awk -v exp_=$exp_d 'BEGIN{ FS=exp_; } {print $2}'`
      tick2=${tick2::-2}
      tick=`perl -e "print sprintf(\"%.2f\", $tick1-$tick2);"`
      echo -e "STKOPT\t$symbol\t$price\t$lotsize\t0.05\t${date_map[$exp_d]}\t10\t$tick\t$tick\t\t10"
      echo "${date_map[$exp_d]}" >>$expiry_days
done

data_source="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
#if grep -aq $symbol $data_source ; then
#    echo "Entry of new symbol exist in datasymbol file"
#    exit
#fi
#grep -v $new_sym $data_source > $tmp_data
#mv  $tmp_data
#echo "Updating datasource exchange symbol"
echo "Add entry to the Datat source file(Manually)->> $data_source"
last_num=`awk -F"NSE" '{print $2}' $data_source | tail -1`
last_num=$((last_num+1))
sort $expiry_days >/tmp/exp_sorted_days
cp /tmp/exp_sorted_days $expiry_days
for exp_d in "${!date_map[@]}";
do
    echo -e "NSE${last_num}\tNSE_${symbol}_FUT_${date_map[$exp_d]}"
    last_num=$((last_num+1))
done
exp_days=($( <$expiry_days))
echo -e "NSE${last_num}\tNSE_${symbol}_FUT_${exp_days[0]}_${exp_days[1]}"
last_num=$((last_num+1))

echo "Checking option for Month $exp_d"
grep $symbol[0-9] $ref_file | grep OPTSTK| grep PE | grep $exp_d >$tmp_file1

for line in `cat $tmp_file1|cut -d' ' -f8`;
do       
        price=`echo $line| awk -v exp_=$exp_d 'BEGIN{ FS=exp_; } {print $2}'`
        price=${price::-2}
        showprice=`perl -e "print sprintf(\"%.2f\", $price);"`
        echo -e "NSE${last_num}\tNSE_${symbol}_PE_${exp_days[0]}_${showprice}"
        last_num=$((last_num+1))
        echo -e "NSE${last_num}\tNSE_${symbol}_PE_${exp_days[1]}_${showprice}"
        last_num=$((last_num+1))
#        echo -e "NSE${last_num}\tNSE_${symbol}_PE_${exp_days[2]}_${showprice}"
#        last_num=$((last_num+1))
        echo -e "NSE${last_num}\tNSE_${symbol}_CE_${exp_days[0]}_${showprice}"
        last_num=$((last_num+1))
        echo -e "NSE${last_num}\tNSE_${symbol}_CE_${exp_days[1]}_${showprice}"
        last_num=$((last_num+1))
#        echo -e "NSE${last_num}\tNSE_${symbol}_CE_${exp_days[2]}_${showprice}"
#        last_num=$((last_num+1))
done

echo "Updating nse_midterm_shortcodes command3--> cat /tmp/filenewShortcode >>/home/pengine/prod/live_configs/nse_midterm_shortcodes"
grep SBIN /home/pengine/prod/live_configs/nse_midterm_shortcodes >/tmp/filenewShortcode
new_sym=$symbol
echo $new_sym
sed -i "s/SBIN/$new_sym/g" /tmp/filenewShortcode
echo
echo "Update Security Margin File will following Values--> /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${next_working_day}.txt"
price=`grep $new_sym "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${next_working_day}.txt"| head -1 |cut -d' ' -f2`
grep SBIN "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${next_working_day}.txt" >/tmp/securitymarginTobeadded
sed -i "s/SBIN/$new_sym/g"  /tmp/securitymarginTobeadded
awk -v pr=$price '{print $1" " pr }' /tmp/securitymarginTobeadded

echo 
echo "After Adding To Contract file Update DB"
echo "Command1--> /apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/Update_MidTermDB_Lotsizes.py  --date $next_working_day --db_path /spare/local/tradeinfo/NSE_Files/midterm_db"
echo
echo "command2--> chown dvctrader:infra /spare/local/tradeinfo/NSE_Files/midterm_db"
