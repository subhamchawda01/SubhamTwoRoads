#!/bin/bash

if [ $# -gt 0 ] ; then
    dstr=$1; shift;

    for SHORTCODE in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FGBX_0 FESX_0 FDAX_0 CGB_0 SXF_0 BAX_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BAX_6 ; 
    do 
# by dd_pnl
	~/infracore/ModelScripts/choose_strats_for_day.pl $SHORTCODE US_MORN_DAY $dstr | grep "Y " | head -10 > ~/choices/US_MORN_DAY_d_$SHORTCODE.$dstr ; 
# by pnl
	~/infracore/ModelScripts/choose_strats_for_day.pl $SHORTCODE US_MORN_DAY $dstr | grep "Y " | sort -rg -k4 | head -10 > ~/choices/US_MORN_DAY_p_$SHORTCODE.$dstr ; 
# by sharpe
	~/infracore/ModelScripts/choose_strats_for_day.pl $SHORTCODE US_MORN_DAY $dstr | grep "Y " | sort -rg -k7 | head -10 > ~/choices/US_MORN_DAY_s_$SHORTCODE.$dstr ; 
	
    done
    
else

    dstr=`date +%Y%m%d`; 
    
    for SHORTCODE in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FGBX_0 FESX_0 FDAX_0 CGB_0 SXF_0 BAX_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BAX_6 ; 
    do 
# by dd_pnl
	~/infracore/ModelScripts/choose_strats_for_day.pl $SHORTCODE US_MORN_DAY | grep "Y " | head -10 > ~/choices/US_MORN_DAY_d_$SHORTCODE.$dstr ; 
# by pnl
	~/infracore/ModelScripts/choose_strats_for_day.pl $SHORTCODE US_MORN_DAY | grep "Y " | sort -rg -k4 | head -10 > ~/choices/US_MORN_DAY_p_$SHORTCODE.$dstr ; 
# by sharpe
	~/infracore/ModelScripts/choose_strats_for_day.pl $SHORTCODE US_MORN_DAY | grep "Y " | sort -rg -k7 | head -10 > ~/choices/US_MORN_DAY_s_$SHORTCODE.$dstr ; 
	
    done
    
fi

find $HOME/choices/US_* -type f -mtime +25 -exec rm -f {} \;
