cd ~/infracore/SysInfo/BloombergEcoReports/ ;
~/basetrade/ModelScripts/us_bond_auction_prep.sh ;
TZ=America/New_York ~/basetrade/ModelScripts/process_us_bond_auction.pl full_auction_history.txt > auction_us_eco_2015_processed.txt ;
sort -g -k1 auction_us_eco_2015_processed.txt > auction_us_eco_2015_processed.txt_1 ; mv auction_us_eco_2015_processed.txt_1 auction_us_eco_2015_processed.txt ;
