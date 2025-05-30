#!/bin/bash
declare -A server_to_file_map
declare -A server_to_oebu_map

server_to_file_map=( ["IND14"]="/var/www/html/market_data_ind14.json" \
                   ["IND19"]="/var/www/html/market_data_ind19.json" \
                   ["IND20"]="/var/www/html/market_data_ind20_options.json" )

server_to_oebu_map=( ["IND14"]="/var/www/html/oebu_info_ind14.json" \
                   ["IND19"]="/var/www/html/oebu_info_ind19.json" \
                   ["IND20"]="/var/www/html/oebu_info_ind20_options.json" )

today=`date +"%Y%m%d"`
prev_tmp=`date -d "yesterday" +"%Y-%m-%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday = "1" ];then
        echo "NSE holiday..., exiting";
        exit
fi

snap_dir="/home/dvcinfra/important/OEBU_SNAPSHOT/$today"
mkdir -p $snap_dir
cd $snap_dir
>IND19
>IND20
>IND14
pwd
while true; do
        for server in "${!server_to_file_map[@]}";
        do
                echo $server
                cat ${server_to_oebu_map[$server]} | sed 's/"//g' | sed 's/}//g' | sed 's/]//g'  | cut -d' ' -f3- >> $server
                echo "SHORTCODE BP AP T_PNL PnlDiff Pos " | awk '{ printf "%-20s %-10s %-10s %-10s %-10s %-10s\n", $1, $2, $3, $4, $5, $6}' >>$server
                echo "" >>$server
                cat ${server_to_file_map[$server]}  | cut -d' ' -f5- | sed 's/\[//g' | tr ']' '\n' | sed 's/"/ /g' | sed 's/}/ /g' | grep -v COMB | cut -c2- | sed 's/ //g' |  awk -F',' '{ if ($15 != 0 )printf "%-20s %-10s %-10s %-10s %-10s %-10s\n", $1, $5, $8, $15, $16, $17}' | sed -r '/^\s*$/d' | sort -nk4 >>$server
                echo "" >>$server
                echo "" >>$server
        
        done 
sleep 30;
done 
