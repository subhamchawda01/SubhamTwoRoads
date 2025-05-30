#!/bin/bash

main_dir="/home/dvctrader/ATHENA/CONFIG_CM_NON_FO_202106/"
back_dir="/home/dvctrader/ATHENA/CONFIG_CM_NON_FO_202106_BKP_20210602/"

cd $main_dir
cat LIVE_FILE_*.csv | grep -i _MM | awk '{print $3}' >/tmp/product_that_exist_in_main_dir

cd $back_dir
ls | grep -f /tmp/product_that_exist_in_main_dir  > /tmp/produt_in_folder_existing_in_bkp

cd $main_dir
cat LIVE_FILE_*.csv | grep -i _MM | awk '{print $3}'  | grep -v -f /tmp/produt_in_folder_existing_in_bkp >/tmp/product_folder_not_exist_in_bkp


if [ -s /tmp/product_folder_not_exist_in_bkp ]
then
   cat /tmp/product_folder_not_exist_in_bkp | mailx  -s "Folder Not exist in Backup NonFO " -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, ravi.parikh@tworoads.co.in 
fi


