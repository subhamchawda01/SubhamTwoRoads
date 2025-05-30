#!/bin/bash
dstr=`date +%Y%m%d`; 

for SHORTCODE in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FESX_0 FDAX_0 ; 
do 
# by pnl
    ~/basetrade/ModelScripts/choose_strats_for_day.pl $SHORTCODE EU_MORN_DAY | grep "Y " | sort -rg -k4 | head -5 > ~/choices/EU_MORN_DAY_p_$SHORTCODE.$dstr ; 
# by sharpe
    ~/basetrade/ModelScripts/choose_strats_for_day.pl $SHORTCODE EU_MORN_DAY | grep "Y " | sort -rg -k7 | head -5 > ~/choices/EU_MORN_DAY_s_$SHORTCODE.$dstr ; 

done

find $HOME/choices/EU_* -type f -mtime +25 -exec rm -f {} \;
