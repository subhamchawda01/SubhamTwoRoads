#!/bin/bash

today=`date +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

#                ["IND12"]="10.23.227.62" \
#                ["IND13"]="10.23.227.63" \

declare -A IND_Server_ip
  IND_Server_ip=( ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND19"]="10.23.227.69" \
                ["IND22"]="10.23.227.71" \
                ["IND20"]="10.23.227.84")

declare -A IND_Server_TMP_FILE
  IND_Server_TMP_FILE=( ["IND14"]="/tmp/oebu_prod_tmp_ind14" \
                ["IND15"]="/tmp/oebu_prod_tmp_ind15" \
                ["IND19"]="/tmp/oebu_prod_tmp_ind19" \
                 ["IND22"]="/tmp/oebu_prod_tmp_ind22" \
                ["IND20"]="/tmp/oebu_prod_tmp_ind20")

declare -A IND_Server_OEBU_FILE
  IND_Server_OEBU_FILE=( ["IND14"]="/spare/local/files/oebu_volmon_product_list_ind14.txt" \
                ["IND15"]="/spare/local/files/oebu_volmon_product_list_ind15.txt" \
                ["IND19"]="/spare/local/files/oebu_volmon_product_list_ind19.txt" \
                ["IND22"]="/spare/local/files/oebu_volmon_product_list_ind22.txt" \
                ["IND20"]="/spare/local/files/oebu_volmon_product_list_ind20_options.txt")

#declare -A IND_Server_OEBU_FILE
#  IND_Server_OEBU_FILE=( ["IND14"]="/tmp/oebu_volmon_product_list_ind14.txt" \
#                ["IND15"]="/tmp/oebu_volmon_product_list_ind15.txt" \
#                ["IND19"]="/tmp/oebu_volmon_product_list_ind19.txt" \
#                ["IND20"]="/tmp/oebu_volmon_product_list_ind20_options.txt")

GetNearestExpiry() {
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${today}
  expiry_count=`cat ${contract_file} | grep IDX | grep -w "$shortcode" | grep "${YYYY}${MM}" | awk -v date=${today} '{if($6>=date)print $6'} | sort | uniq | wc -l`
  expiry=`cat ${contract_file} | grep IDX | grep -w "$shortcode" | awk -v date=${today} '{if($6>=date)print $6'} | sort | uniq | head -n1`
}

YYYYMMDD=$today ; 
DD=${YYYYMMDD:6:2} 
MM=${YYYYMMDD:4:2} 
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
shortcode_fin="FINNIFTY"
GetNearestExpiry
expiry_fin=$expiry
expiry_fin_count=$expiry_count
shortcode_bnf="BANKNIFTY"
GetNearestExpiry
expiry_bnf=$expiry
expiry_bnf_count=$expiry_count
shortcode_nf="NIFTY"
GetNearestExpiry
expiry_nf=$expiry
expiry_nf_count=$expiry_count

for server in "${!IND_Server_ip[@]}";
do
  >/tmp/oebu_filter_product
  echo "for ---- $server";
  cp ${IND_Server_OEBU_FILE[$server]} /tmp/
  if [ "$server" = "IND22" ]; then
    if [ $expiry_bnf_count == "1" ]; then
      grep "_BANKNIFTY_" ${IND_Server_OEBU_FILE[$server]} | grep -v "_W" >> /tmp/oebu_filter_product
    else
      grep "_BANKNIFTY_" ${IND_Server_OEBU_FILE[$server]} | egrep '_W|_FUT' >> /tmp/oebu_filter_product
    fi
    if [ $expiry_fin_count == "1" ]; then
      grep "_FINNIFTY_" ${IND_Server_OEBU_FILE[$server]} | grep -v "_W" >> /tmp/oebu_filter_product
    else
      grep "_FINNIFTY_" ${IND_Server_OEBU_FILE[$server]} | egrep '_W|_FUT' >> /tmp/oebu_filter_product
    fi
    cat /tmp/oebu_filter_product > ${IND_Server_OEBU_FILE[$server]}
  elif [ "$server" = "IND20" ]; then
    if [ $expiry_nf_count == "1" ]; then
      grep "_NIFTY_" ${IND_Server_OEBU_FILE[$server]} | grep -v "_W" >> /tmp/oebu_filter_product
    else
      grep "_NIFTY_" ${IND_Server_OEBU_FILE[$server]} | egrep '_W|_FUT' >> /tmp/oebu_filter_product
    fi
    cat /tmp/oebu_filter_product > ${IND_Server_OEBU_FILE[$server]}
  fi
  echo -e "NSE_NIFTY50\nNSE_NIFTYFINSERVICE\nNSE_NIFTYBANK\nNSE_NIFTYSMLCAP100\nNSE_NIFTYMIDCAP100" >> ${IND_Server_OEBU_FILE[$server]}
  sort -u ${IND_Server_OEBU_FILE[$server]} > /tmp/oebu_filter_product
  mv /tmp/oebu_filter_product ${IND_Server_OEBU_FILE[$server]} 
done

