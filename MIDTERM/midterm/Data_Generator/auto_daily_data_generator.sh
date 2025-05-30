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

#Obtain the date
if [ $# -eq 0 ];
then
        date_="$(date +'%Y%m%d')"
else
        date_=$1
fi

body="\nTried to generate fut/opt data for date:\t"$date_"\n"
/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "MidTerm fut/opt data gen process triggered for date : $date_"

cd /home/dvctrader/midterm/Data_Generator

rm Sorted_Data/*
rm Sorted_Options_Data/*
rm Sorted_Weekly_Options_Data/*
rm Sorted_Cash_Data/*
rm FILENAMES


bash fut_data_gen.sh $date_ > Logs/fut_logs
exit_status=$?
if [ $exit_status -eq 1 ]
then 
	body=$body"Fut data expiries not found"
	echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
	/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
	rm /home/dvctrader/midterm/Data_Generator/is_running
	exit
fi
if [ $exit_status -eq 2 ]
then
        body=$body"Fut data files not found"
        echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
	/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
	rm /home/dvctrader/midterm/Data_Generator/is_running
	exit
fi

bash opt_data_gen.sh $date_ > Logs/opt_logs
exit_status=$?
if [ $exit_status -eq 1 ]
then
        body=$body"Opt data expiries not found"
        echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
	/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
	rm /home/dvctrader/midterm/Data_Generator/is_running
        exit
fi
if [ $exit_status -eq 2 ]
then
        body=$body"Opt data files not found"
        echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
	/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
	rm /home/dvctrader/midterm/Data_Generator/is_running
        exit
fi

bash weekly_opt_data_gen.sh $date_ > Logs/weekly_option_logs
exit_status=$?
if [ $exit_status -eq 1 ]
then
        body=$body"Weekly Opt data expiries not found"
        echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
	/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
	rm /home/dvctrader/midterm/Data_Generator/is_running
        exit
fi
if [ $exit_status -eq 2 ]
then
        body=$body"Weekly Opt data files not found"
        echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
	/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
	rm /home/dvctrader/midterm/Data_Generator/is_running
        exit
fi

#bash cash_data_gen.sh $date_ > Logs/cash_logs
#exit_status=$?
#if [ $exit_status -eq 1 ]
#then
#    body=$body"Cash data files not found"
#    echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads.co.in
#    /home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
#    rm /home/dvctrader/midterm/Data_Generator/is_running
#    exit
#fi
bash auto_merger.sh >Logs/merge_logs 2>Logs/merge_logs
exit_status=$?
if [ $exit_status -eq 0 ]
then 
	body=$body"Data generated successfully."
else 
	body=$body"FATAL!! Data generated but not merged due to copy failure.\t."
fi

echo -e $body | /bin/mail -s "Data gen" akik.dey@tworoads-trading.co.in
/home/dvctrader/midterm/Data_Generator/send_slack_notification midterm-data-gen DATA "$body"
if [ $exit_status -eq 0 ]
then 
	rm /home/dvctrader/midterm/Data_Generator/is_running
fi

