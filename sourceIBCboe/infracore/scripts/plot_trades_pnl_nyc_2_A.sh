#!/bin/bash

mkdir -p ~/pnltemp
sort $@ > ~/pnltemp/pnl_all_cat_file
~/infracore/scripts/plot_trades_pnl_nyc_2.pl ~/pnltemp/pnl_all_cat_file A
rm -f ~/pnltemp/pnl_all_cat_file
