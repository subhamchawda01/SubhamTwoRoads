#!/usr/bin/bash
mail_report="/tmp/mail_file.txt"

if [ $# -ne 1 ] ; then
  echo "Called As : " $* "<Date>" ;
  exit
fi
today_date=$1
echo "Running for the day $today_date..."
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
contract_difference_file="/tmp/contract_diff_$today_date.txt"
eod_pnl_file="/tmp/ind_pnl_${today_date}.txt"
contract_file="/home/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$today_date"
difference_file="/tmp/difference.txt"
updated_pnl_file="/tmp/updated_ind_pnl_$today_date"
>$updated_pnl_file

#Copying today's difference_contract_file from IND12 in dvcinfra@10.23.5.26
scp dvctrader@10.23.227.62:$contract_difference_file ${contract_difference_file}
if [ -s $contract_difference_file ]; then
   echo "Entries in contract difference file exists. Updating LOTS sizes in today's eod_pnls file"
else
   echo "Entries in contract difference file does not exist. Existing"
   exit;
fi

#Creating backup of the today's dated file in 10.23.5.26
cp /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${today_date}.txt /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${today_date}.txt_bkp_orig
cp /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${today_date}.txt $eod_pnl_file


#Reading the contract_diff_{today_yyyymmdd_} to make updations in the ind_pnl_$today_date.txt
while IFS=" " read -r security expiry new_lot old_lot
        do
		ratio=$((old_lot / new_lot))
	        echo "Sec: $security Ratio: $ratio Lot: $old_lot NLot: $new_lot "
		cat $contract_file | grep $security | grep -v 'OPT' | awk '{print $6}' | sort | uniq > /tmp/security_expiry
	
		exp_no=`grep -nar $expiry /tmp/security_expiry | cut -d':' -f1`
		exp_no=$((exp_no-1))
		fut=$security 
		opt_put=$security
		opt_call=$security
		if [ $exp_no = "0" ]
		then
			fut+="_FUT0"
			opt_put+="_P0"
			opt_call+="_C0"
		elif [ $exp_no = "1" ]
		then
			fut+="_FUT1"
                        opt_put+="_P1"
                        opt_call+="_C1"
		else
			fut+="_FUT2"
                        opt_put+="_P2"
                        opt_call+="_C2"
		fi

		echo "SECURITIES WHOSE LOT SIZE IS TO BE ALTERED $fut $opt_put $opt_call"		

		#Updating the lot size of the security both for F&O
		#Final Working implementation
		awk -v var="$fut" -v fact="$ratio" '$2==var {$29=$29*fact} {print}' $eod_pnl_file > $updated_pnl_file	
		cp $updated_pnl_file $eod_pnl_file

		#Replacing the LOT SIZE of OPTIONS
		awk -v var="$opt_put" -v fact="$ratio" '$2 ~ var {$29=$29*fact} {print}' $eod_pnl_file > $updated_pnl_file
		cp $updated_pnl_file $eod_pnl_file

		awk -v var="$opt_call" -v fact="$ratio" '$2 ~ var {$29=$29*fact} {print}' $eod_pnl_file > $updated_pnl_file
		cp $updated_pnl_file $eod_pnl_file
	
done < "$contract_difference_file"
# Copying the updated file back to original location
cp $updated_pnl_file /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${today_date}.txt
diff -w /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${today_date}.txt_bkp_orig $eod_pnl_file > /tmp/difference.txt

echo '<div style="display:flex;justify-content:space-around">' >$mail_report
echo "<div style='margin:0 30px;'>" >>$mail_report
echo "<table border='1' id='myTable' class='table table-striped' style='border-collapse: collapse'<thead><tr><th>UNDERLYING</th>  <th>OLD_LOT</th> <th>NEW_LOT</th> </tr></thead><tbody>" >> $mail_report
count=`wc -l < $difference_file`
i=2
while [ $i -le $count ]
do

    underlying=`head -n $i $difference_file | tail -n1 | awk '{print $3}'`
    prev=`head -n $i $difference_file | tail -n1 | awk '{ print $30 }'`
   i=$(($i+2))
    new=`head -n $i $difference_file | tail -n1 | awk '{ print $30 }'`
    echo -e "<tr><td>$underlying</td><td>$prev</td><td>$new</td></tr>" >> $mail_report
    i=$(($i+2))
done

echo "</tbody></table></div></body></html>" >> $mail_report
(
# echo "To: tarun.joshi@tworoads-trading.co.in"
  echo "To: raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, tarun.joshi@tworoads-trading.co.in, naman.jain@tworoads-trading.co.in"
  echo "Content-Type: text/html; "
  echo Subject: LOTSIZES FOR EOD_PNL FILES UPDATED
  echo
  cat /tmp/mail_file.txt
) | /usr/sbin/sendmail -t
