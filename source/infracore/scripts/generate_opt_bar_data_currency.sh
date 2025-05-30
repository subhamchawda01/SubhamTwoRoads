#!/bin/bash
#!/bin/tcsh

print_msg_and_exit() {
  echo $* ;
  exit ;
}

[ $# -eq 4 ] || print_msg_and_exit "Usage : < script > < CD/FO/SPOT > < PROD_FILENAME> <DATE> < NO_OF_WORKERS >"


type_=$1
prod_file=$2
date_=$3
workers_=$4

echo "TYPE: $type_ LIST_FILE: $prod_file DATE: $date_ Workers: $workers_"
commands_=""
non_commands_=""
while IFS= read -r var
do
 non_commands_+=$"$type_ $var $date_"'\n'
done < "$prod_file"
echo $type_

#echo $non_commands_ 
#exit
echo -e "$non_commands_" | xargs -P $workers_ -n 3 bash /home/dvctrader/stable_exec/scripts/opt_data_gen_currency.sh
