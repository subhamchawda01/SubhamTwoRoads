cd ~/infracore/SysInfo/BloombergEcoReports ;
~/basetrade/ModelScripts/process_bloomberg_eco.pl bbg_us_eco_2015.csv > bbg_us_eco_2015_processed.txt
replace "Bloomberg_Consumer_Comfort 2" "Bloomberg_Consumer_Comfort 0" -- bbg_us_eco_2015_processed.txt
replace "Bloomberg_Economic_Expectations 2" "Bloomberg_Economic_Expectations 0" -- bbg_us_eco_2015_processed.txt
replace "Bloomberg_News_Launches_Consumer_Comfort_Index 2" "Bloomberg_News_Launches_Consumer_Comfort_Index 0" -- bbg_us_eco_2015_processed.txt
replace "RBC_Consumer_Outlook_Index 2" "RBC_Consumer_Outlook_Index 0" -- bbg_us_eco_2015_processed.txt
replace "Challenger_Job_Cuts_YoY 2" "Challenger_Job_Cuts_YoY 0" -- bbg_us_eco_2015_processed.txt
