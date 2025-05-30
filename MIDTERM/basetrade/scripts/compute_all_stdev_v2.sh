#!/bin/bash

# BRZ
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BR_DOL_0 BRT_900 BRT_1800 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BR_IND_0 BRT_900 BRT_1800 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BR_WIN_0 BRT_900 BRT_1800 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BR_WDO_0 BRT_900 BRT_1800 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1F15 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1F16 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1F17 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1F18 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1F21 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1F22 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1N15 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1N16 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1J15 BRT_900 BRT_1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl DI1J16 BRT_900 BRT_1500

for prod in `cat /spare/local/files/BMF/stock_list`; 
do
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl $prod BRT_1005 BRT_1655
done

# CME US
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl ZN_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl ZB_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl ZF_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl UB_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl ES_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NQ_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl YM_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl GE_4 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl GE_5 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl ZW_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6A_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6B_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6C_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6E_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6J_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6N_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6M_0 1200 2000
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl 6S_0 1200 2000


# EUREX EU
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FGBL_0 700 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FGBM_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FGBS_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FGBX_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FESX_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FDAX_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FOAT_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FOAM_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FBTP_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FBTS_0 700 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl FESB_0 700 1600

# LIFFE US
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl LFR_0 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl YFEBM_0 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl YFEBM_1 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl YFEBM_2 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XFC_0 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XFC_1 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XRFC_0 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XRFC_1 1200 1600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl LFZ_0 1200 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl KFFTI_0 1200 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl JFFCE_0 1200 1600

# LIFFE EU
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl LFR_0 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl YFEBM_0 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl YFEBM_1 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl YFEBM_2 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XFC_0 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XFC_1 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XRFC_0 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl XRFC_1 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl LFZ_0 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl ZW_0 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl KFFTI_0 800 1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl JFFCE_0 800 1600

# RTS/MICEX EU
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl RI_0 0610 1945
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl Si_0 0610 1945
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl USD000UTSTOM 0610 1945

# HKEX
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl HHI_0 0130 0400
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl HSI_0 0130 0400
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl MHI_0 0130 0400
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl HHI_0 0500 0830
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl HSI_0 0500 0830
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl MHI_0 0500 0830
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl HHI_0 1200 1500 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl HSI_0 1200 1500
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl MHI_0 1200 1500

# OSE 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NK_0 010 0600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NKM_0 010 0600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NKM_1 010 0600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NK_0 0800 1800
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NKM_0 0800 1800
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NKM_1 0800 1800
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NKMF_0 010 0600 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl JGBL_0 010 0600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl TOPIX_0 010 0600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl NKMF_0 0800 1800
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl JGBL_0 0800 1800
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl TOPIX_0 0800 1800

# TMX 
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl SXF_0 EST_930 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl CGB_0 EST_600 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BAX_0 EST_600 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BAX_1 EST_600 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BAX_2 EST_600 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BAX_3 EST_600 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BAX_4 EST_600 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BAX_5 EST_600 EST_1600
$HOME/basetrade/scripts/compute_prod_stdev_v2.pl BAX_6 EST_600 EST_1600
