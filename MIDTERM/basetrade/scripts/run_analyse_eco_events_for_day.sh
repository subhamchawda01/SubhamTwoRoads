#!/bin/bash

if [ $USER == "sghosh" ] || [ $USER == "ravi" ] || [ $USER == "rahul" ] || [ $USER == "diwakar" ] ;
then
    DATE="TODAY";

    # # Setup the modelling repo.
    # cd $HOME/modelling ;
    # git pull >/dev/null 2>&1;
    # $HOME/modelling/change_paths_to_self.sh >/dev/null 2>&1;

    # # Setup infra to get today's events.
    # cd $HOME/infracore ;
    # git pull >/dev/null 2>&1;
    # rm -f $HOME/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2012_processed.txt >/dev/null 2>&1;
    # bjam release link=static -j16 >/dev/null 2>&1;

    echo " >>>>>>>>>>>>> $HOME/basetrade_install/bin/economic_events_of_the_day $DATE";
    $HOME/basetrade_install/bin/economic_events_of_the_day $DATE;

#   for SHORTCODE in HHI_0 HSI_0 MHI_0 FGBM_0 FGBS_0 FDAX_0 FGBL_0 FESX_0 FOAT_0 FBTP_0 LFR_0 LFZ_0 KFFTI_0 JFFCE_0 XFC_0 YFEBM_1 CGB_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BR_DOL_0 BR_IND_0 BR_WIN_0 BR_DI_3 BR_DI_5 ZF_0 ZN_0 UB_0 ;
    for SHORTCODE in FGBM_0 FGBS_0 FGBL_0 FOAT_0
    do 
	echo " >>>>> $HOME/basetrade/ModelScripts/analyse_eco_events_for_day.pl $SHORTCODE US_MORN_DAY $DATE TODAY-80 TODAY-1" ;
	$HOME/basetrade/ModelScripts/analyse_eco_events_for_day.pl $SHORTCODE US_MORN_DAY $DATE TODAY-80 TODAY-1 ;
    done

    cd $HOME/modelling ;
    git checkout strats stratwork >/dev/null 2>&1;
fi
