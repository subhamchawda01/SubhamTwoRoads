#!/bin/bash


/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/circularScript.py >/tmp/modified_circular_update_from_pyt_nse 2>&1

echo "Data fetched"

#This is used to create the circular_update_prev.txt file if it doesn't exists
if [ -f /tmp/circular_update_prev.txt ]
then
        echo ""
else
        > /tmp/circular_update_prev.txt;
fi

if [[ $1 == "NU" ]]; then 
  if grep "No Updates!" /tmp/modified_circular_update_from_pyt_nse; then
         echo "NU Exiting from the code"
         exit
  fi
fi
#echo "HAPPY HAPPY"

cat /tmp/modified_circular_update_from_pyt_nse > /tmp/circular_update_new.txt;



>/tmp/circular_update_mail_file.txt


while read line; do

        str=`grep "$line" /tmp/circular_update_prev.txt`;

	if [ -z "$str" ]
	then
		echo $line >> /tmp/circular_update_mail_file.txt;
	fi

done < "/tmp/circular_update_new.txt"



cp /tmp/circular_update_new.txt /tmp/circular_update_prev.txt;


if [ -s /tmp/circular_update_mail_file.txt ]
	then
        echo "Sending mail"
#      	cat /tmp/circular_update_mail_file.txt | mailx -s "NSE Circular updates" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in;
        cat /tmp/circular_update_mail_file.txt | mailx -s "NSE Circular updates" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  subham.chawda@tworoads-trading.co.in nseall@tworoads.co.in smit@tworoads-trading.co.in
else 
       echo "No new updates.. Sending mail"
#       echo "No New Updates..." | mailx -s "NSE Circular updates" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in;
        echo "No New Updates..." | mailx -s "NSE Circular updates" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  subham.chawda@tworoads-trading.co.in nseall@tworoads.co.in smit@tworoads-trading.co.in
fi

rm /tmp/circular_update_new.txt;
rm /tmp/circular_update_mail_file.txt;

