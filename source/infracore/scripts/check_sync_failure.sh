#! /bin/bash

#update_and_reload_events.sh script runs every day thrice between 12:03AM and 1:00AM GMT 
#to sync the newly fetched events on all the servers and ec2 workers.
#this script can be run on any of the servers/workers to detect a sync failure.
#the script detects the sync failure by checking whether or not the file was last modified between 12:03AM-1:30AM GMT

#sets the year to current year for which merged_eco_file has to be fetched
year=`date '+%Y'`;

#gets the file modification date in YYYYMMDD format
date_modify="$(ls -l --full-time --time-style='+%Y%m%d %H %M %S' /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_"$year"_processed.txt | awk 'BEGIN{tmp=0; sec=0;} {tmp=$6; sec=$8*60+$7*3600+$9 } END{print tmp;}')";
#gets the file modification date in seconds from epoch
date_modify_sec="$(date +"%s" --date=$date_modify)";
#gets the file modification time in seconds
time_modify_sec="$(ls -l --full-time --time-style='+%Y%m%d %H %M %S' /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_"$year"_processed.txt | awk 'BEGIN{sec=0;} { sec=$8*60+$7*3600+$9 } END{print sec;}')";
#gets the modification timestamp as seconds from epoch
res="$(echo $(($date_modify_sec+$time_modify_sec)))";
#gets the start time on or aftr which the file is expected to be updated
#the time here is hardcoded as 12:03AM GMT, ie 180 seconds from midnight
start_time="$(DATE=`date '+%Y%m%d'` | date +"%s" --date=$DATE |  awk 'BEGIN{st=0;} {st=$1;} END{print st; }')";
#gets the end time on or before which the file is expected to be updated
#the end time here is hardcoded as 1:30AM GMT, ie 5400seconds from midnight
end_time="$(DATE=`date '+%Y%m%d'` | date +"%s" --date=$DATE |  awk 'BEGIN{end=0;} {end=$1+86400;} END{print end; }')";
#checks and returns true if file is non empty and was updated between the start and end times as expected
if [ -e /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_"$year"_processed.txt ] && [ -f  /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_"$year"_processed.txt  ] && [ -s  /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_"$year"_processed.txt  ] && [ $res -ge $start_time ] && [ $res -le $end_time ] ; then echo "1"; else echo "0" ; fi

