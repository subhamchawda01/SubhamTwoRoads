#!/bin/bash

if [ $# -ne 2 ] ; then
  echo "USAGE:<SCRIPT> <PRODUCT> <VALUE>"
  exit
fi

DATE=`date +%Y%m%d`
PROD=$1
VALUE=$2
NEW_SYM_FILE="/tmp/new_${PROD}_sym_file_${DATE}"
data_source="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"

>${NEW_SYM_FILE}
echo "Add entry to the Datat source file(Manually)->> $data_source"
echo "NEW Symbol File Path : $NEW_SYM_FILE"
last_num=`awk -F"NSE" '{print $2}' $data_source | tail -1`
echo "last_num: $last_num"
((last_num++))

egrep "${PROD}_CE_|${PROD}_PE_" $data_source | awk -v strt=$last_num -v val=$VALUE -v date_=$DATE -F "_" '{if($4>=date_){printf "NSE%d\tNSE_%s_%s_%s_%.2f\n",strt,$2,$3,$4,($5-val);strt++}}' >${NEW_SYM_FILE}
