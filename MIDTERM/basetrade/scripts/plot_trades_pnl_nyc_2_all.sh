#!/bin/bash

mkdir -p ~/pnltemp
date_=`echo $1 | cut -d'.' -f2`
sort $@ > ~/pnltemp/pnl_all_cat_file.$date_
~/basetrade/scripts/plot_trades_pnl_nyc_2.pl ~/pnltemp/pnl_all_cat_file.$date_
rm -f ~/pnltemp/pnl_all_cat_file.$date_
