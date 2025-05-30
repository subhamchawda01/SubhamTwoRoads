#!/bin/bash

USAGE="<script> <Indicator> <dest_dir> <prod_list=/spare/local/tradeinfo/curr_trade_prod_list.txt>";

if [ $# -lt 2 ]; then echo $USAGE; exit 0; fi;

ind=$1; shift;
dest_dir=$1; shift;
prod_list="/spare/local/tradeinfo/curr_trade_prod_list.txt";
if [ $# -gt 0 ]; then prod_list=$1; shift; fi;

for shc in `cat $prod_list`; do for strat in $HOME/modelling/*strats/$shc/*/*; do if [ -s $strat ] ; then model=`awk '{print $4}' $strat` ; if [ -s $model ]; then if [ `grep -c " $ind " $model` -gt 0 ]; then echo $strat ; fi; fi; fi; done > $dest_dir/$shc; done
