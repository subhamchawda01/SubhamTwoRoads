#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

init(){

NSE_FO_DATA_CONVERTER=/home/dvcinfra/LiveExec/bin/NSE_CONVERT_AND_FILTER_FO_DATA 
[ -f $NSE_FO_DATA_CONVERTER -a -s $NSE_FO_DATA_CONVERTER -a -r $NSE_FO_DATA_CONVERTER ] || print_msg_and_exit "NSE_FO DATA CONVERTER -> $NSE_FO_DATA_CONVERTER EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

FILTERED_PRODUCT_LIST_FILE=/home/dvcinfra/LiveExec/config/pair_trading_filtered_products_list.txt
[ -f $FILTERED_PRODUCT_LIST_FILE -a -s $FILTERED_PRODUCT_LIST_FILE -a -r $FILTERED_PRODUCT_LIST_FILE ] || print_msg_and_exit "NSE_FO DATA FILTER LIST FILE -> $FILTERED_PRODUCT_LIST_FILE EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

RAW_FUT_DATAFILE="/spare/local/RawData/dump_raw_multicast_data_using_config_nse_tbt_nf_sf_so_secondary" ;
[ -f $RAW_FUT_DATAFILE -a -s $RAW_FUT_DATAFILE -a -r $RAW_FUT_DATAFILE ] || print_msg_and_exit "NSE_FO RAW DATAFILE -> $RAW_FUT_DATAFILE EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT REDABLE" ;

today=`date +"%Y%m%d"` ;

time $NSE_FO_DATA_CONVERTER $RAW_FUT_DATAFILE $today $FILTERED_PRODUCT_LIST_FILE

}

init ;
