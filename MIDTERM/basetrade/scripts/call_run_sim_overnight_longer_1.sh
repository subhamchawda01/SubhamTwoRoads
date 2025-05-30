#!/bin/bash

cd $HOME

if [ -e $HOME/.gcc_profile ] ; 
then 
    source $HOME/.gcc_profile ; 
else
    export NEW_GCC_LIB=/usr/local/lib
    export NEW_GCC_LIB64=/usr/local/lib64
    export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH
fi

CRSOP_SCRIPT=$HOME/LiveExec/scripts/call_run_sim_overnight_perdir_longer.pl ;
if [ ! -e $CRSOP_SCRIPT ] ; then 
CRSOP_SCRIPT=$HOME/basetrade_install/scripts/call_run_sim_overnight_perdir_longer.pl ;
fi

#if [ ! -e $HOME/locks/call_run_sim_overnight_longer.lock ] ; then
#touch $HOME/locks/call_run_sim_overnight_longer.lock ;

# high value longer
#$CRSOP_SCRIPT BR_DOL_0 4 15
#$CRSOP_SCRIPT CGB_0 4 15
$CRSOP_SCRIPT FGBS_0 4 15
$CRSOP_SCRIPT FGBM_0 4 15
$CRSOP_SCRIPT FGBL_0 4 15
$CRSOP_SCRIPT ZN_0 4 15
$CRSOP_SCRIPT FBTS_0 4 15
$CRSOP_SCRIPT FOAT_0 4 15
$CRSOP_SCRIPT ZB_0 4 15

$CRSOP_SCRIPT HHI_0 4 15
$CRSOP_SCRIPT HSI_0 4 15
$CRSOP_SCRIPT MHI_0 4 15

#$CRSOP_SCRIPT NK_0 4 15
#$CRSOP_SCRIPT NKM_0 4 15

$CRSOP_SCRIPT UB_0 4 15
$CRSOP_SCRIPT ZF_0 4 15


#
#$CRSOP_SCRIPT BR_IND_0 4 15
#$CRSOP_SCRIPT BR_WIN_0 4 15
#$CRSOP_SCRIPT DI1F15 4 15
#$CRSOP_SCRIPT DI1F16 4 15
#$CRSOP_SCRIPT DI1F17 4 15
#$CRSOP_SCRIPT DI1F18 4 15
#$CRSOP_SCRIPT DI1F19 4 15
#$CRSOP_SCRIPT DI1F20 4 15
#$CRSOP_SCRIPT DI1N14 4 15
#$CRSOP_SCRIPT DI1N15 4 15
#$CRSOP_SCRIPT DI1N16 4 15
#$CRSOP_SCRIPT DI1N17 4 15
#$CRSOP_SCRIPT BR_DI1N_4 4 15
#$CRSOP_SCRIPT BR_DI1N_5 4 15
#$CRSOP_SCRIPT BR_DI1G_0 4 15
#$CRSOP_SCRIPT BR_DI1G_1 4 15
#$CRSOP_SCRIPT BR_DI1G_2 4 15
#$CRSOP_SCRIPT DI1J15 4 15
#$CRSOP_SCRIPT DI1J16 4 15
#$CRSOP_SCRIPT DI1J17 4 15
#
$CRSOP_SCRIPT LFI_0 4 15
$CRSOP_SCRIPT LFI_1 4 15
$CRSOP_SCRIPT LFI_2 4 15
$CRSOP_SCRIPT LFI_3 4 15
$CRSOP_SCRIPT LFI_4 4 15
$CRSOP_SCRIPT LFI_5 4 15
$CRSOP_SCRIPT LFI_6 4 15

$CRSOP_SCRIPT KFFTI_0 4 15
$CRSOP_SCRIPT JFFCE_0 4 15
$CRSOP_SCRIPT LFZ_0 4 15
$CRSOP_SCRIPT LFR_0 4 15
$CRSOP_SCRIPT YFEBM_1 4 15
$CRSOP_SCRIPT XFC_0 4 15

#
$CRSOP_SCRIPT FESX_0 4 15
$CRSOP_SCRIPT FBTP_0 4 15
$CRSOP_SCRIPT FDAX_0 4 15

$CRSOP_SCRIPT SXF_0 4 15
$CRSOP_SCRIPT FGBX_0 4 15
# less used daily ones:
#~/basetrade/scripts/call_run_sim_overnight_perdir.pl RI_0
#~/basetrade/scripts/call_run_sim_overnight_perdir.pl Si_0
#~/basetrade/scripts/call_run_sim_overnight_perdir.pl ED_0
#~/basetrade/scripts/call_run_sim_overnight_perdir.pl GD_0

#~/basetrade/scripts/call_run_sim_overnight_perdir.pl USD000UTSTOM

#$CRSOP_SCRIPT RI_0 4 15
#$CRSOP_SCRIPT Si_0 4 15
#$CRSOP_SCRIPT ED_0 4 15
#$CRSOP_SCRIPT GD_0 4 15

#$CRSOP_SCRIPT USD000UTSTOM 4 15

#rm -f $HOME/locks/call_run_sim_overnight_longer.lock ;
#else
#    echo "$HOME/locks/call_run_sim_overnight_longer.lock present";
#fi
