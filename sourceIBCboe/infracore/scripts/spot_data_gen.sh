#!/bin/bash

tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
datasource_exchsymbol_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
log_dir="/spare/local/"
bar_data_dir="${log_dir}BarData_IDX/"
shc_exec="${log_dir}get_shortcode_for_symbol_from_file"
bar_data_exec="/home/dvctrader/stable_exec/nse_historical_bar_data_generator_20210929"

print_msg_and_exit() {
  echo $* ;
  exit ;
}

[ $# -eq 3 ] || print_msg_and_exit "Usage : < script > < CD/FO/SPOT > < PROD[NIFTY50]> <DATE>"

is_cd=$1
prod=$2
dt=$3
ref_data_file="/spare/local/tradeinfo/NSE_Files/RefData/nse_cd_${dt}_contracts.txt"

is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $dt T`
if [ $is_holiday_ = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

START_TIME="IST_915"
END_TIME="IST_1530"
start_=$(date -d "$dt 0345" +%s)
end_=$(date -d "$dt 1000" +%s)

bar_data_dir="${log_dir}BarData_SPOT/"
lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' ${bar_data_dir}${prod})
echo "Line_num: $lineNum"
if [ -z "$lineNum" ]
then
        echo "no corrupt data for $prod"
else
        sed -i "$lineNum,\$d" "${bar_data_dir}${prod}"
fi

#sym_file="/tmp/prod_spot_name_sym"

echo "${bar_data_exec} ${dt} NSE_${prod} ${START_TIME} ${END_TIME} 1 ${bar_data_dir}  "
${bar_data_exec} ${dt} <(echo "NSE_${prod}") ${START_TIME} ${END_TIME} 1 ${bar_data_dir}  
echo "${prod} DONE"
