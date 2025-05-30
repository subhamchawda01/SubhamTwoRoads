#!/bin/bash

mkdir -p ~/pnltemp
sort $@ > ~/pnltemp/pnl_all_cat_file
~/basetrade/scripts/plot_trades_pnl_utc_2.pl ~/pnltemp/pnl_all_cat_file A
rm -f ~/pnltemp/pnl_all_cat_file
