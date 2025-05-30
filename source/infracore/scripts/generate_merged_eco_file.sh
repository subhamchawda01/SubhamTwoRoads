cd /home/pengine/fxstreet;
year=`date +%Y` ;
perl /home/pengine/prod/live_scripts/union_bbg_fxs_usauctions.pl bbg_us_eco_$year"_processed.txt" fxstreet_eco_$year"_processed.txt" auction_us_eco_$year"_processed.txt" wasde_us_eco_$year"_processed.txt" merged_eco_$year"_processed.txt" ; 
mv merged_eco_$year"_processed.txt" merged_eco_$year"_processed.txt_1" ; 
sort -s -k1 merged_eco_$year"_processed.txt_1" | uniq > merged_eco_$year"_processed.txt" ; 
#rm -f merged_eco_2016_processed.txt_1 ; #Temporarily commented
replace "Dallas_Fed_Manf._Activity 2" "Dallas_Fed_Manf._Activity 0" -- merged_eco_$year"_processed.txt" ; 
