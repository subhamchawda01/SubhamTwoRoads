#!/bin/bash

product_ind13="/tmp/product_trading_ind13"
product_ind21="/tmp/product_trading_ind21"
sec_server="10.23.227.66"

get_sym="/home/pengine/prod/live_execs/get_symbol_from_shortcode_file"
comm_product_files="/tmp/product_trading_all"
mach_trade_prod="/tmp/products_trading_machine"
symbol_file="/tmp/products_trading_file_pe"
live_prod_no_data="/home/dvcinfra/important/live_prod_no_data.txt"
>$live_prod_no_data

declare -A IND_Server_ip
#IND_Server_ip=(["IND16"]="10.23.227.81")

IND_Server_ip=( ["IND14"]="10.23.227.64"
                ["IND15"]="10.23.227.65"
                ["IND16"]="10.23.227.81"
                ["IND17"]="10.23.227.82"
                ["IND18"]="10.23.227.83"
                ["IND22"]="10.23.227.71"
                ["IND23"]="10.23.227.72"
                ["IND19"]="10.23.227.69"
                ["IND20"]="10.23.227.84")

date_=`date +"%Y%m%d"`;
ls /spare/local/MDSlogs/GENERIC | grep NSE | grep -v '^ORS_' >$product_ind13
ls /spare/local/MDSlogs/GENERIC_NIFTY | grep NSE | grep -v '^ORS_' >>$product_ind13
ssh $sec_server "ls /spare/local/MDSlogs/GENERIC | grep NSE | grep -v ORS_" >$product_ind21
ssh $sec_server "ls /spare/local/MDSlogs/GENERIC_NIFTY | grep NSE | grep -v ORS_" >>$product_ind21
cat $product_ind13 >$comm_product_files
cat $product_ind21 >>$comm_product_files



for server in "${!IND_Server_ip[@]}";
do
    echo "for ---- $server";
    ssh dvctrader@${IND_Server_ip[$server]} "cat /spare/local/logs/tradelogs/log.${date_}.* | grep SecurityMarketView | awk '{ print \$3}'" >$mach_trade_prod
    count_=`cat $mach_trade_prod | wc -l`
    LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1  $get_sym $mach_trade_prod $date_  | grep -v "Not Valid ShortCode" >$symbol_file
    echo "$server "
    while IFS= read -r line; do
#        echo "Text read from file: $line"
        file_=`echo $line | cut -d' ' -f2`
        sym_=`echo $line | cut -d' ' -f1`
#        echo "File: $file_"
        if ! grep -q "${file_}" $comm_product_files
        then
          echo "$sym_ Update not Exist"
          echo "$server $sym_ $file_" >>$live_prod_no_data
        else
          echo "$sym_ Update Exist"
        fi
    done < $symbol_file
done

cp /home/dvcinfra/important/live_prod_no_data.txt /home/dvcinfra/important/live_prod_no_data.txt_tmp
cat /home/dvcinfra/important/live_prod_no_data.txt_tmp | sort | uniq | awk '{if ($2 in a) {a[$2]=a[$2]","$1;b[$2]=$3 } else {a[$2]=$1;b[$2]=$3}} END {for (i in a) {print a[i],i,b[i]}}' >/home/dvcinfra/important/live_prod_no_data.txt

/home/pengine/prod/live_scripts/check_data_mtbt_for_live_webpage.sh
