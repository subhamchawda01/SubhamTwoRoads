#!/bin/bash
#!/bin/tcsh
# generates fut0 bar data , fut1 data expiry and day before expiry


GetNearestExpiry() {	
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${date_}
  expiry=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${date_} '{if($NF>=date)print $NF'} | sort | uniq | head -n1`
}

GenBarDataProducts() {
  >${products_file}
  cat ${tradeinfo_dir}'/Lotsizes/fo_mktlots_'${date_}'.csv' \
	| awk  -F "," '{print $2}' \
	| awk -v suffix=$shortcode_suffix '{print "NSE_"$1"_"suffix}'  > ${products_file}
}

RemoveCurruptedData() {
  for file in `ls ${bardata_output_dir}`;
  do 
    cat ${bardata_output_dir}$file | awk -v start_time=${start_timestamp} '{if ($1<start_time){print $0}}' > ${bardata_output_dir}/tmp_${file}
    [ ! -f ${bardata_output_dir}/tmp_${file} ] && continue;
    mv ${bardata_output_dir}/tmp_${file} ${bardata_output_dir}/${file}
  done
}

[ $# -ne 1 ] && { echo "PROVIDE DATE IN THE ARGUMENT"; exit; }

date_=$1
shortcode_suffix="FUT0";
products_file='/tmp/bar_data_products'
start_timestamp=`date +"%s" -d "${date_}"`
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
bardata_output_dir='/spare/local/BarData/'
GetNearestExpiry
[ ${expiry} == ${date_} ] && shortcode_suffix="FUT1" 

GenBarDataProducts

if [ ! -f ${products_file} ]  || [ ! -s ${products_file} ];
then
	echo "FAILED GENERATING PRODUCTS FILES, EXITING...";
	exit
fi

RemoveCurruptedData

LD_PRELOAD=/home/dvctrader/libcrypto.so.1.1 /home/dvctrader/stable_exec/nse_historical_bar_data_generator ${date_} ${products_file} IST_915 IST_1530  1 ${bardata_output_dir}

[ $? -ne 0 ] && { echo "FAILED BAR DATA GENERATION"; exit; }
