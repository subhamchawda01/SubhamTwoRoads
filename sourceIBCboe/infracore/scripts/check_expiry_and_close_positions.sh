#!/bin/bash

date_=`date +%Y%m%d`
month_=`date +%m`
contract_file="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.${date_}"
tmp_file="/tmp/date_list_to_expiry_of_banknifty_check"

grep BANKNIFTY $contract_file  | awk '{print $6}' | sort | uniq > $tmp_file

if grep -q $date_ $tmp_file;then
       month_date=`date '+%m' -d "+7 days"`
       if [[ ${month_date#0} -eq ${month_#0} ]]; then
                echo "Weekly Expiry:- $date_"
                echo "" | mailx -s "Reminder Weekly Expiry: $date_ Close BANKNIFTY & NIFTY Opt Positions" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nseall@tworoads.co.in
#expiry_=`date +%Y%m%d`;cat /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${expiry_}.txt   | grep -v "TOTAL_POS:      0" | grep -v FUT | grep BANKNIFTY | awk '{if(NF>20)print $0}' | awk '{ if($(NF-6)>0){ print $(NF-1),"1",$(NF-6)} else {print $(NF-1),"0",-$(NF-6)}}' | awk '{print $1"\001"$2"\001"$3"\0010\001211\0014833858800542177300\0011568864998.428204\00120578417"}' >/NAS1/data/MFGlobalTrades/ind_pnls/FO/trades_to_adjust/trades.${expiry_}
       else 
                echo "Monthly Expiry:- $date_"
                echo "" | mailx -s "Reminder Monthly Expiry: $date_ Close All Opt Positions" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nseall@tworoads.co.in

#expiry_=`date +%Y%m%d`;cat /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${expiry_}.txt   | grep -v "TOTAL_POS:      0" | grep -v FUT | awk '{if(NF>20)print $0}' | awk '{ if($(NF-6)>0){ print $(NF-1),"1",$(NF-6)} else {print $(NF-1),"0",-$(NF-6)}}' | awk '{print $1"\001"$2"\001"$3"\0010\001211\0014833858800542177300\0011568864998.428204\00120578417"}' >/NAS1/data/MFGlobalTrades/ind_pnls/FO/trades_to_adjust/trades.${expiry_}
       fi
fi

echo "Check Fin Expiry"

grep FINNIFTY $contract_file  | awk '{print $6}' | sort | uniq > $tmp_file

if grep -q $date_ $tmp_file;then
   echo "FINNIFTY Weekly Expiry:- $date_"
   echo "" | mailx -s "FINNIFTY Reminder Expiry: $date_ Close Opt Positions" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nseall@tworoads.co.in
fi

