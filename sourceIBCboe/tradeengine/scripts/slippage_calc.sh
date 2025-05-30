#!/bin/bash
logfile_=$1
sq_data_=$(grep "UNCASCADE\|SQUAREOFF" $logfile_  | grep -v Activating | grep -v Offset | sort -k1,1 -s)
sl_data_=$(grep "HITTING STOP LOSS" $logfile_  | awk -F "[][{}]" '{print $1,$2,$3}'| awk '{print $5,"UNCASCADE",($11+$13)/2,1.00}')
sq_data_=$(echo "${sq_data_}
${sl_data_}" | sort -k1,1 -s)
# echo $sq_data_
casc_data_=$(grep CASCADE $logfile_ | grep -v UNCASCADE | awk '{print $1,$4,$8}'|sort -k1,1 -s)
theo_data_=$(grep TheoOnExec $logfile_ | awk '{print $2,$6,$8,$5}'|sort -k1,1 -s)
send_data_=$(grep SEND $logfile_ | awk '{print $2,$8}'|sort -k1,1 -s)
prod_data_=$(grep TheoOnExec $logfile_ | awk '{print $2}'|sort -k1,1 -s | uniq)
readarray -t sq_lines < <(echo "$sq_data_")
readarray -t casc_lines < <(echo "$casc_data_")
readarray -t theo_lines < <(echo "$theo_data_")
readarray -t send_lines < <(echo "$send_data_")
readarray -t prod_list_ < <(echo "$prod_data_")

theo_id_="0"
casc_id_="0"
sq_id_="0"
send_id_="0"
count_="0"
count_1_="0"
count_2_="0"
slip_="0"
slip_1_="0"
slip_2_="0"
# echo "${sq_lines[17]}" "sq"
# echo "${casc_lines[$casc_id_]}" "casc"
# echo "${theo_lines[theo_id_]}" "theo"
# echo "${prod_list_[1]}" "prod"
for i in "${!prod_list_[@]}"; do 
	while [[ "${theo_lines[theo_id_]}" != "${prod_list_[$i]}"*  ]]
	do
  		theo_id_=`expr $theo_id_ + 1`
	done
	# echo "${theo_lines[theo_id_]}" "$theo_id_"
	while [[ "${casc_lines[casc_id_]}" != "${prod_list_[$i]}"* ]]
	do
  		casc_id_=`expr $casc_id_ + 1`
	done
	# echo "${casc_lines[casc_id_]}" 
	while [[ "${sq_lines[sq_id_]}" != "${prod_list_[$i]}"* ]]
	do
  		sq_id_=`expr $sq_id_ + 1`
	done

	#while [[ "${send_lines[send_id_]}" != "${prod_list_[$i]}"*  ]]
	#do
  	#	send_id_=`expr $send_id_ + 1`
	#done
	# echo "${sq_lines[sq_id_]}" 
	trade_=$(echo "${theo_lines[theo_id_]}" | cut -d' ' -f2)
	prev_lot_size_="0"
	#trade_size_=$(echo "${send_lines[send_id_]}" | cut -d' ' -f2)
	#trade_size_=${trade_size_#-}
	casc_id_=`expr $casc_id_ + 1`
	counter_="0"
	# echo "${prod_list_[$i]}" 
	while [[ "${theo_lines[theo_id_]}" == "${prod_list_[$i]}"*  ]]
	do
		current_trade_=$(echo "${theo_lines[theo_id_]}" | cut -d' ' -f2)

		if [[ "$current_trade_" == "$trade_" ]]; then
			lot_size_=$(echo "${theo_lines[theo_id_]}" | cut -d' ' -f4)
			lot_size_=$(echo $lot_size_ - $prev_lot_size_|bc -l)
			# echo "$lot_size_" "$prev_lot_size_"
			prev_lot_size_=$(echo $lot_size_+$prev_lot_size_|bc -l)
			lot_size_=${lot_size_#-}
			#multiplier_=$(echo $trade_size_/$lot_size_|bc -l)	
			multiplier_=1
			# echo "bye"
			cascade_px_=$(echo "${casc_lines[casc_id_]}" | cut -d' ' -f2)
			exec_px_=$(echo "${theo_lines[theo_id_]}" | cut -d' ' -f3)
			ratio_=$(echo "${casc_lines[casc_id_]}" | cut -d' ' -f3)
			
			val_=$(echo $cascade_px_ $exec_px_ $ratio_| awk '{print $1/$3,$2}' | awk '{printf("%f",($2-$1)/$1)}')
			if [[ "$trade_" == "SELL" ]]; then
				val_=$(echo $val_*-1|bc -l)
			fi
			counter_=`expr $counter_ + 1`
			if(( $(echo "$counter_ == $multiplier_" | bc -l) ));  then
				casc_id_=`expr $casc_id_ + 1`
				counter_="0"
			fi
			count_=`expr $count_ + 1`
			count_1_=`expr $count_1_ + 1`
			# echo $slip_ $val_
			slip_=$(echo $slip_ + $val_|bc -l)
			slip_1_=$(echo $slip_1_ + $val_|bc -l)
			echo ${prod_list_[$i]} $cascade_px_ $exec_px_ $val_ $slip_ $slip_1_
		else
			# echo "hi\n"
			sq_px_=$(echo "${sq_lines[sq_id_]}" | cut -d' ' -f3)
			exec_px_=$(echo "${theo_lines[theo_id_]}" | cut -d' ' -f3)
			ratio_=$(echo "${sq_lines[sq_id_]}" | cut -d' ' -f4)
			val_="0"
			prev_lot_size_="0"
			
			val_=$(echo $sq_px_ $exec_px_ $ratio_| awk '{print $1/$3,$2}' | awk '{printf("%f",($1-$2)/$1)}')
			count_=`expr $count_ + 1`
			count_2_=`expr $count_2_ + 1`
			if [[ "$trade_" == "SELL" ]]; then
				val_=$(echo $val_*-1|bc -l)
			fi
			# echo $slip_ $val_
			slip_=$(echo $slip_ + $val_|bc -l)
			slip_2_=$(echo $slip_2_ + $val_|bc -l)
			echo ${prod_list_[$i]} $sq_px_ $exec_px_ $val_ $slip_ $slip_2_
			# sq_id_=`expr $sq_id_ + 1`
		fi
		
		# echo $slip_ $val_
		# slip_=$(echo $slip_ + $val_|bc -l)
		# count_=`expr $count_ + 1`
  		theo_id_=`expr $theo_id_ + 1`
	done
	# echo $trade_

	# printf "%s %s %s %s \n" "$i" "${prod_list_[$i]}" "${theo_lines[theo_id_]}" "${casc_lines[casc_id_]}"  "${sq_lines[sq_id_]}"
done
# echo $slip_
slip_=$(echo $slip_/$count_|bc -l)
slip_1_=$(echo $slip_1_/$count_1_|bc -l)
slip_2_=$(echo $slip_2_/$count_2_|bc -l)
echo $slip_ $slip_1_ $slip_2_
