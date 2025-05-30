#!/bin/bash

if [ $USER == "rkumar" ] || [ $USER == "ravi" ] ;
then
    for SHORTCODE in FGBS_0 FGBM_0 FESX_0 FGBL_0 FOAT_0 FDAX_0 CGB_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BR_DOL_0 BR_WIN_0 BR_IND_0 ZF_0 ZN_0 ZB_0 UB_0 JFFCE_0 KFFTI_0 LFR_0 LFZ_0 YFEBM_0 YFEBM_1 YFEBM_2 ;
    do
	DATE=`$HOME/infracore/scripts/call_get_iso_date_from_str_min1.pl TODAY-1`;

	$HOME/infracore_install/bin/ors_binary_reader $SHORTCODE $DATE SUMMARY 2>&1 | grep -v "not exist";
    done
fi