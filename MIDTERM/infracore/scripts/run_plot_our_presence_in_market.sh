#!/bin/bash

if [ $USER == "rkumar" ] || [ $USER == "ravi" ] ;
then
    for SHORTCODE in FGBS_0 FGBM_0 FESX_0 FGBL_0 FOAT_0 CGB_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BR_DOL_0 BR_WIN_0 BR_IND_0 ZF_0 ZN_0 ZB_0 UB_0 ;
    do
	$HOME/infracore/scripts/plot_our_presence_in_market.pl $SHORTCODE US_MORN_DAY TODAY-2 SUMMARY VR;
	$HOME/infracore/scripts/plot_our_presence_in_market.pl $SHORTCODE US_MORN_DAY TODAY-1 SUMMARY VR;
    done
fi