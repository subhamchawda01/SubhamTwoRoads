#!/bin/bash

YYYYMMDD=`date +%Y%m%d` 
file_check=/tmp/order_msg_details_$YYYYMMDD
process_file="/tmp/order_process_mssg_details_$YYYYMMDD"
> $process_file
reject_handling_sc="/home/pengine/prod/live_scripts/orsRejectHandling.sh"
touch $file_check

handle_reject(){
	echo "Handling ShortCode"
	type_=`echo $line  | cut -d' ' -f1`;
	if [[ $type_ == "Reject:" ]]; then
		shortcode_=`echo $line  | cut -d' ' -f2`;
		echo "HANDLE: $reject_handling_sc addts $shortcode_"
		$reject_handling_sc addts $shortcode_
	elif [[ $type_ == "ALERT" ]]; then
		shortcode_=`echo $line  | cut -d' ' -f10`;
		echo "HANDLE: $reject_handling_sc maxorder $shortcode_"
		$reject_handling_sc maxorder $shortcode_
	fi
	echo "Handled"
}

while true; do
	echo "READING: "
	while read line; do 
		echo "ERROR: $line"
		shortcode_count=`echo $line | grep NSE_ | wc -l`
		[[ $shortcode_count -eq 0 ]] && continue;
		handle_reject
	done < $file_check
	>$file_check
	echo "SLEEPING: "
	sleep 10
done
