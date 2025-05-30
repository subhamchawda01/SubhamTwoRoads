#!/bin/bash

path_="/spare/local/logs/alllogs/MediumTerm/"
today=`date +"%Y%m%d"`;

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday = "1" ] ; then
        echo "NSE Holiday. Exiting...";
        exit;
fi

file_disp=$path_"bkp_trades_disp.dat"
file_midterm=$path_"bkp_trades_midterm.dat"
file_manual=$path_"bkp_trades_manual.dat"

file_disp_bkp=$path_"bkp_trades_disp.dat_$today"
file_midterm_bkp=$path_"bkp_trades_midterm.dat_$today"
file_manual_bkp=$path_"bkp_trades_manual.dat_$today"

echo "File $file_disp $file_disp_bkp"
echo "File $file_midterm $file_midterm_bkp"
echo "File $file_manual $file_manual_bkp"
echo "Moving files"
mv $file_disp $file_disp_bkp
mv $file_midterm $file_midterm_bkp
mv $file_manual $file_manual_bkp

echo "Updating"

egrep -v 'leq|geq|abs_' $file_disp_bkp >$file_disp
egrep -v 'leq|geq|abs_' $file_midterm_bkp >$file_midterm
egrep -v 'leq|geq|abs_' $file_manual_bkp >$file_manual

date_=`date +%Y%m%d`
month_=`date +%m`
contract_file="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.${date_}"
tmp_file="/tmp/date_list_to_expiry_of_banknifty_check"

grep BANKNIFTY $contract_file  | awk '{print $6}' | sort | uniq > $tmp_file

if grep -q $date_ $tmp_file;then
       month_date=`date '+%m' -d "+7 days"`
       if [[ $month_date -eq $month_ ]]; then
                echo "Weekly Expiry:- $date_"
		echo "updating Options"
                sed -i 's/_1_/_0_/g' $file_disp
		sed -i 's/_1_/_0_/g' $file_manual
		sed -i 's/_1_/_0_/g' $file_midterm
                echo "" | mailx -s "Notional Weekly Expiry: $date_" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in smit@tworoads-trading.co.in

       else
                echo "Monthly Expiry:- $date_"
		echo "REMOVE FUT0 $file_disp $file_manual $file_midterm"
		sed '/FUT0/d' $file_disp
		sed '/FUT0/d' $file_manual
		sed '/FUT0/d' $file_midterm
		echo "Udpate FUT1 -> FUT0"
		sed -i 's/FUT1/FUT0/g' $file_disp
                sed -i 's/FUT1/FUT0/g' $file_manual
                sed -i 's/FUT1/FUT0/g' $file_midterm
		echo "Updateing Options"
		sed -i 's/_1_/_0_/g' $file_disp
                sed -i 's/_1_/_0_/g' $file_manual
                sed -i 's/_1_/_0_/g' $file_midterm
                echo "" | mailx -s "Notional Monthly Expiry: $date_" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in smit@tworoads-trading.co.in
       fi
fi

