#!/bin/bash

OEBU_SHC_FILE="/tmp/oebu_shc";
OEBU_SHC_LIST="/tmp/oebu_req_list"
> ${OEBU_SHC_LIST}
NSESHM_log_file="/spare/local/MDSlogs/nseshm_writer_log.`date +%Y%m%d`"
echo $NSESHM_log_file
cat /spare/local/files/oebu_volmon_product_list_ind14.txt >${OEBU_SHC_FILE}; 
cat /spare/local/files/oebu_volmon_product_list_ind15.txt >>${OEBU_SHC_FILE}; 
cat /spare/local/files/oebu_volmon_product_list_ind16.txt >>${OEBU_SHC_FILE};
cat /spare/local/files/oebu_volmon_product_list_ind17.txt >>${OEBU_SHC_FILE};
cat /spare/local/files/oebu_volmon_product_list_ind18.txt >>${OEBU_SHC_FILE};
cat /spare/local/files/oebu_volmon_product_list_ind19.txt >>${OEBU_SHC_FILE};
cat /spare/local/files/oebu_volmon_product_list_ind20_options.txt >>${OEBU_SHC_FILE};
cat /spare/local/files/oebu_volmon_product_list_ind23.txt >>${OEBU_SHC_FILE};

for prod in `cat ${OEBU_SHC_FILE}`; 
do 
  if [ `grep -w $prod ${NSESHM_log_file} | wc -l` == "0" ]; 
  then 
    echo $prod >> ${OEBU_SHC_LIST}; 
  fi 
done
sort -u ${OEBU_SHC_LIST} > /tmp/oebu_tmp_req ;
mv /tmp/oebu_tmp_req ${OEBU_SHC_LIST}

LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1 /home/pengine/prod/live_execs/shortcode_helper ${OEBU_SHC_LIST} > /tmp/oebu_sc_helper_log 2>&1
