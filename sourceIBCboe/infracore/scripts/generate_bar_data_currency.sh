#!/bin/bash
#!/bin/tcsh
date_=$1
prod_file=$2
workers_=$3
type_=$4
expiry_date_="0"
fut1_expiry_date_="0"
fut2_expiry_date_="0"
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'

contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${date_}
expiry_date_=`cat ${contract_file} | grep CURFUT | grep GBPUSD | awk -v date=${date_} '{if($NF>=date)print $NF'} | sort | uniq | head -n1`
echo $expiry_date_

non_commands_=""
while IFS= read -r var
do
 non_commands_+=$"$date_ $var $expiry_date_"'\n'
done < "$prod_file"
echo "TYPE: $type_"
echo "$non_commands_"
echo -e "$non_commands_" | xargs -P $workers_ -n 3 bash /home/dvctrader/stable_exec/scripts/fut_data_gen_currency.sh
