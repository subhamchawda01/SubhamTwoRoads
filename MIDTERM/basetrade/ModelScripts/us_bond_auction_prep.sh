#!/bin/bash

MODELSCRIPTS_DIR=$HOME/basetrade_install/ModelScripts;

wget -N http://www.treasury.gov/resource-center/data-chart-center/quarterly-refunding/Documents/auctions.pdf
# tlm=`stat -c%Y auctions.pdf`;

# cdt=`date +%s`;
# #cdt=`stat -c%Y auctions.txt`;

# #if [ $tlm -ge $cdt ] ; 
# if [ $tlm -ge $(($cdt-90000)) ] ; 
# then
    pdftotext -layout auctions.pdf 
    cat auctions.txt | sed 's# R ##g' | sed 's# T ##g' | awk '{ if ( NF > 5 ) { print $0 }}' | egrep "NOTE|BOND|TIPS" | awk '{printf "%s %s %s %s\n", $1,$8,$9,$10}' > new_auction_history.txt
    
    $MODELSCRIPTS_DIR/add_only_new_auctions.pl new_auction_history.txt full_auction_history.txt;
# fi