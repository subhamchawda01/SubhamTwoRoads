#!/bin/bash

#USAGE 1: ./run_pick_strat_and_install.sh [short_code1 short_code2 .. ]   # to run for specific products
#USAGE 2: ./run_pick_strat_and_install.sh                                 # to run for all products. 
#USAGE 3: ./run_pick_strat_and_install.sh - [ shortcode ]  # to run in EU hours
# No need to change the script.
cd $HOME/modelling;
git pull;

SHC="";
if [ "$1" == "-" ]; then 
    shift;
    SHCEU="FGBS_0 FGBM_0 FGBL_0 FESX_0 LFR_0 KFFTI_0 FOAT_0 FBTP_0 ZF_0 ZB_0 ZN_0 UB_0"    
    if [ $# -gt 0 ]; then SHCEU=$*; fi
    cd $HOME/modelling;
    git pull;
    shift
    if [ $# -gt 1 ]; then shortcodes_=$*; else shortcodes_=$SHCEU; fi
    a="for SHORTCODE in $shortcodes_; do $HOME/basetrade/ModelScripts/pick_strats_and_install.pl \$SHORTCODE EU_MORN_DAY $HOME/modelling/pick_strats_config/EU/\$SHORTCODE.EU.txt; done"; 
    echo $a; 
    for SHORTCODE in $shortcodes_; do $HOME/basetrade/ModelScripts/pick_strats_and_install.pl $SHORTCODE EU_MORN_DAY $HOME/modelling/pick_strats_config/EU/$SHORTCODE.EU.txt; done 
    exit; 
fi;
if [ $# -gt 0 ] ; 
then 
    SHC=$* ; 
    if [ "$SHC" == "LFI" ] ; then SHC="LFI_0 LFI_1 LFI_2 LFI_3 LFI_4 LFI_5 LFI_6 LFI_7 LFI_8 LFI_9"; 
    elif [ "$SHC" == "LFL" ] ; then SHC="LFL_0 LFL_1 LFL_2 LFL_3 LFL_4 LFL_5 LFL_6"; 
    elif [ "$SHC" == "BAX" ] ; then SHC="BAX_0 BAX_1 BAX_2 BAX_3 BAX_4";
    elif [ "$SHC" == "DI1" ]; then SHC="DI1F15 DI1F16 DI1F17 DI1F18 DI1F19"; 
    fi
else
    SHC="\
FGBS_0 FGBM_0 FGBL_0 FOAT_0 FBTP_0 FBTS_0 \
BR_IND_0 BR_WIN_0 BR_DOL_0 \
FESX_0 CGB_0 ZF_0 ZN_0 ZB_0 UB_0 \
LFR_0 KFFTI_0 JFFCE_0 LFZ_0 \
BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 "
# NKM_0 NK_0 \ kp will monitor for few days..
# DI1F15 DI1F16 DI1F17 DI1F18 DI1N15 "
# LFI_0 LFI_1 LFI_2 LFI_3 LFI_4 LFI_5 LFI_6 LFL_2 LFL_3 LFL_4 LFL_5 LFL_6"
fi
echo "Running pick strat for: $SHC";

if [ $USER != "dvctrader" ] ; then
    echo "USER should be dvctrader. Probably should check for the host also";
else
    for SHORTCODE in $SHC;
    do
	if [ ${SHORTCODE::2} == "NK" ] ;
	then
	    TIME_STR=US_MORN_DAY
	    echo " >>>>> $HOME/basetrade/ModelScripts/pick_strats_and_install.pl $SHORTCODE $TIME_STR $HOME/modelling/pick_strats_config/AS/$SHORTCODE.US.txt" ;
	    $HOME/basetrade/ModelScripts/pick_strats_and_install.pl $SHORTCODE $TIME_STR $HOME/modelling/pick_strats_config/AS/$SHORTCODE.US.txt ;
	elif [ -e $HOME/modelling/pick_strats_config/US/$SHORTCODE.US.txt ] ; then
	    TIME_STR=US_MORN_DAY
	    if [[ ${SHORTCODE::3} =~ LF[IL] ]]; then TIME_STR=EUS_MORN_DAY; fi;
	    echo " >>>>> $HOME/basetrade/ModelScripts/pick_strats_and_install.pl $SHORTCODE $TIME_STR $HOME/modelling/pick_strats_config/US/$SHORTCODE.US.txt" ;
	    $HOME/basetrade/ModelScripts/pick_strats_and_install.pl $SHORTCODE $TIME_STR $HOME/modelling/pick_strats_config/US/$SHORTCODE.US.txt ;
	else
	    echo " For $SHORTCODE US_MORN_DAY config "$HOME"/modelling/pick_strats_config/US/"$SHORTCODE".US.txt missing" ;
	fi
    done
fi
