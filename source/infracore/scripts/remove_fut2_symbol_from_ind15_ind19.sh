#!/bin/bash


GetNearestExpiry() {
    contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${today}
      expiry=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${today} '{if($NF>=date)print $NF'} | sort | uniq | head -n1`
}


tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
today=`date +%Y%m%d`;
#today=20211230;
GetNearestExpiry


if [ ${expiry} == ${today} ] ; then
  grep -v _FUT2 /spare/local/files/oebu_volmon_product_list_ind15.txt >/tmp/tmp_oebu_txt
  cp /tmp/tmp_oebu_txt /spare/local/files/oebu_volmon_product_list_ind15.txt

  grep -v _FUT2 /spare/local/files/oebu_volmon_product_list_ind19.txt >/tmp/tmp_oebu_txt
  cp /tmp/tmp_oebu_txt /spare/local/files/oebu_volmon_product_list_ind19.txt

fi

