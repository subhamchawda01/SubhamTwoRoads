#!/bin/bash

if [ $USER == "sghosh" ] || [ $USER == "ravi" ] ;
then
    if [ ! -e $HOME/locks/run_find_optimal_intervals_to_pick_queries.lock ] ; then
	touch $HOME/locks/run_find_optimal_intervals_to_pick_queries.lock ;

    cd $HOME/modelling;
    git pull;

    rsync -avz --delete dvctrader@10.23.74.51:/home/dvctrader/ec2_globalresults $HOME;

    for SHORTCODE in BAX_2 BAX_3 BAX_4 BAX_5 CGB_0 BR_DOL_0 BR_IND_0 BR_WIN_0 ZF_0 ZN_0 ZB_0 UB_0 ZT_0 FESX_0 FGBS_0 FGBM_0 FGBL_0 ; 
    do 
	echo " >>>>> $HOME/basetrade/ModelScripts/find_optimal_intervals_to_pick_queries.pl $SHORTCODE US_MORN_DAY $HOME/modelling/pick_strats_config/sghosh/$SHORTCODE.sghosh.permute.txt TODAY-50 TODAY" ;
	$HOME/basetrade/ModelScripts/find_optimal_intervals_to_pick_queries.pl $SHORTCODE US_MORN_DAY $HOME/modelling/pick_strats_config/sghosh/$SHORTCODE.sghosh.permute.txt TODAY-50 TODAY;
    done

    rm -f $HOME/locks/run_find_optimal_intervals_to_pick_queries.lock ;
    else
	echo "$HOME/locks/run_find_optimal_intervals_to_pick_queries.lock present";
    fi
fi