#!/bin/bash
GetNearestExpiry() {
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${date_}
  expiry=`cat ${contract_file} | grep IDX | grep $shortcode | awk -v date=${date_} '{if($6>=date)print $6'} | sort | uniq | head -n1`
}
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'

date_=`date +%Y%m%d`
#date_="20220406"
folder_to_sync="/home/dvctrader/ATHENA/CONFIG_OPT_IDX_STRANGLE_ALL_RUSH"
shortcode="BANKNIFTY"
GetNearestExpiry
if [[ $date_ == $expiry ]]; then
    folder_to_sync="/home/dvctrader/ATHENA/CONFIG_OPT_IDX_STRANGLE_ALL_EXPIRY_RUSH"
fi

echo "Folder To Sync : $folder_to_sync"
while true; do
  echo "Sycning the File from IND20"
# rsync -avz dvctrader@10.23.227.84:/home/dvctrader/ATHENA/CONFIG_OPT_IDX_ATM_STRADDLE_SPOT_IBB_HDG_LONGSPLIT /home/dvcinfra/important/
  echo "rsync -avz dvctrader@10.23.227.71:$folder_to_sync /home/dvcinfra/important/STRADDLE_FILES/$date_"
  rsync -avz dvctrader@10.23.227.71:$folder_to_sync /home/dvcinfra/important/STRADDLE_FILES/$date_
  status1=$?
  [ $status1 -ne 0 ] && { echo "Sync Failed to IND20 Retrying..."; sleep 1m; continue;  }
  break;
done


#find /home/dvcinfra/important/STRADDLE_FILES/* -mtime +7 -exec rm {} \;
