#!/bin/bash


date_=`date +%Y%m%d`
month_=`date +%m`
contract_file="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.${date_}"
tmp_file="/tmp/date_list_to_expiry_of_banknifty_check"

grep BANKNIFTY $contract_file  | awk '{print $6}' | sort | uniq > $tmp_file
echo "RUN"

if grep -q $date_ $tmp_file;then
       month_date=`date '+%m' -d "+7 days"`
       if [[ $month_date -eq $month_ ]]; then
                echo "Weekly Expiry:- $date_"
                cp "/spare/local/files/NSE/MidTermLogs/EODPosFiles/$date_" /spare/local/files/NSE/MidTermLogs/EODPosFiles/${date_}_org_bkp
                grep -v _0_ "/spare/local/files/NSE/MidTermLogs/EODPosFiles/$date_"  > /home/dvctrader/trash/POS_$date_
                sed -i 's/_1_/_0_/g' /home/dvctrader/trash/POS_$date_
                cp /home/dvctrader/trash/POS_$date_ "/spare/local/files/NSE/MidTermLogs/EODPosFiles/$date_"
                echo "" | mailx -s "Updating Midterm Weekly Expiry: $date_ and closing _0" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in smit@tworoads-trading.co.in

       else
                echo "Monthly Expiry:- $date_"
                cp "/spare/local/files/NSE/MidTermLogs/EODPosFiles/$date_" /spare/local/files/NSE/MidTermLogs/EODPosFiles/${date_}_org_bkp
                /home/dvctrader/anaconda3/bin/python /home/dvctrader/basequant/Scripts/ApplyExpiryFixes.py ${date_}
                sed -i 's/_1_/_0_/g' "/spare/local/files/NSE/MidTermLogs/EODPosFiles/$date_"
                echo "" | mailx -s "Updating Midterm Monthly Expiry: $date_ Closing Old pos" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in smit@tworoads-trading.co.in
       fi
fi

