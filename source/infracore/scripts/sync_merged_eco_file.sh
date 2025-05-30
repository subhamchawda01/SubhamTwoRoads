#!/bin/bash
#To be run only on dvctrader@ny11

year=`date "+%Y"` 

#cd to events_infracore/infracore/ and pull there (to avoid any issue due to local edits)
cd /home/dvctrader/events_infracore/infracore; git reset --hard; git pull && git submodule foreach git pull; cd -;

#we are looking at tradeinfo directory for events
cp -p /home/dvctrader/events_infracore/infracore/SysInfo/BloombergEcoReports/merged_eco_$year"_processed.txt" /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_$year"_processed.txt"

#Set permissions for the file to give read access to group and others
chmod 644 /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_$year"_processed.txt"

#sync the file on all machines
/home/dvctrader/infracore/scripts/sync_file_to_all_machines.pl /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_$year"_processed.txt"
/home/dvctrader/basetrade/AwsScripts/sync_single_file_2_ec2.sh /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_$year"_processed.txt" ;


