#!/bin/bash
#args=("$@")

#Making sure it doesn't run if another instance is already running
if [ -f /home/dvctrader/midterm/Data_Generator/is_running ]
then
	body="One instance is already running, cannot run again"
	echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
	exit
fi
touch /home/dvctrader/midterm/Data_Generator/is_running

cd /home/dvctrader/midterm/Data_Generator

rm Sorted_Data/*
rm Sorted_Options_Data/*
rm Sorted_Weekly_Options_Data/*
rm FILENAMES

for date_ in `cat missing_dates`; do
	echo "\nGenerating for date:\t"$date_"\n"
	bash fut_data_gen.sh $date_ > Logs/fut_logs
	bash opt_data_gen.sh $date_ > Logs/opt_logs
	bash weekly_opt_data_gen.sh $date_ > Logs/weekly_option_logs
done

#bash auto_merger.sh > Logs/merge_logs
if [ $exit_status -eq 0 ]
then 
body=$body"Data generated successfully !! :)"
else 
body=$body"ALERT!!!! Data generated but not merged. Some copy failure has occured.\t Need immediate manual intervention. Please take a look asap."
fi

echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
rm /home/dvctrader/midterm/Data_Generator/is_running
