today=$1;
less /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${today}.txt  | grep "FUT" | awk '{print $20*$26/100000}' | awk '{sum+=$1; abssum+= sqrt($1*$1)} END {print "GROSS EXPOSURE: ",abssum," \nNET EXPOSURE: ",sum}' | mail -s "FO Exposure: $today" ravi.parikh@tworoads.co.in uttkarsh.sarraf@tworoads.co.in nishit.bhandari@tworoads.co.in
