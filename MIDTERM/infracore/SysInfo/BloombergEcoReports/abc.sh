cd ~/infracore/SysInfo/BloombergEcoReports ; 
~/basetrade/ModelScripts/union_bbg_fxs_usauctions.pl bbg_us_eco_2012_processed.txt ../FXStreetEcoReports/fxstreet_eco_2012_processed.txt auction_us_eco_2012_processed.txt merged_eco_2012_processed.txt ; 
mv merged_eco_2012_processed.txt merged_eco_2012_processed.txt_1 ; 
sort -s -k1 merged_eco_2012_processed.txt_1 | uniq > merged_eco_2012_processed.txt ; 
rm -f merged_eco_2012_processed.txt_1 ;
replace "Dallas_Fed_Manf._Activity 2" "Dallas_Fed_Manf._Activity 0" -- merged_eco_2012_processed.txt ; 
