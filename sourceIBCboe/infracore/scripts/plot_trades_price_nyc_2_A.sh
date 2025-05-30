#!/bin/bash

mkdir -p ~/pnltemp
sort $@ > ~/pnltemp/pnl_all_cat_file
~/infracore/scripts/plot_trades_price_utc.pl ~/pnltemp/pnl_all_cat_file
rm -f ~/pnltemp/pnl_all_cat_file
