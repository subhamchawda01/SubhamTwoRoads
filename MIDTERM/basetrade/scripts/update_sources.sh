#!/bin/bash
#updates /spare/local/tradeinfo/sources.txt file, which is used to select the shortcodes for which various offline info has to be computed

uid=`date +%N`;
tmpfile=tmpfile_"$uid";
for i in $HOME/modelling/indicatorwork/prod_configs/comb_config_*; do cat $i | awk '{if($1=="SELF" || $1=="SOURCE"){$1=""; print $_;}}'; done > $tmpfile;
grep -v "PLINE HYB_" /spare/local/tradeinfo/PCAInfo/portfolio_inputs | awk '{$1="";$2="";print $_}' >> $tmpfile;
cat /spare/local/tradeinfo/sources.txt >> $tmpfile;
for i in `cat $tmpfile`; do echo $i; done | sort | uniq | grep -v DUMMY_SHC > /spare/local/tradeinfo/sources.txt;
rm -f $tmpfile;

