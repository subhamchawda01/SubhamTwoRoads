#!/bin/bash

mkdir -p ~/pnltemp
hours_offset=14400;
hostserver=`hostname`

if [ "$hostserver" == "SDV-HK-SRV12" ] || [ "$hostserver" == "SDV-HK-SRV11" ] 
then 

    hours_offset=0; 

fi 

sort $@ | awk '{ printf "%.6f", ($1-'$hours_offset'); for ( i=2; i <= NF; i ++ ) { printf " %s", $i; } printf "\n"; }' > ~/pnltemp/pnl_all_cat_file
~/basetrade/scripts/plot_trades_price_utc.pl ~/pnltemp/pnl_all_cat_file
rm -f ~/pnltemp/pnl_all_cat_file
