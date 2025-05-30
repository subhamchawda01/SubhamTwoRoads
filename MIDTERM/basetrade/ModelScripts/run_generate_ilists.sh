#!/bin/bash

if [ $# -lt 1 ] ; 
then 
    echo "$0 shortcode_ config_"
    echo "$0 shortcode_"
    exit;
elif [ $# == 1 ] ;
then
    shc=$1; shift;
    config="NA";
elif [ $# == 2 ] ;
then
    shc=$1; shift;
    config=$1; shift;
fi

#for now TODO make correlation cutoff maps # CGB_0 min_indcator ~= 200 FBTP ~= 100 
#dc 1.5 => dont remove indicators which are correlated with each other ( this part is handled by remove_correlated_indicators , 3rd step of generate_ilists.pl ) 
#im max_number from output base_ilists.pl 
#-ic used in remove_correlated
#remove correlation 

common_string="-im 1000 -sd TODAY-400 -ed TODAY-1 -cc 1.5 -ic 0.8 -ss SHARPE -cs TODAY-4 -ce TODAY-2 -mm 5000 -rm 300 -nm 1";


#REPO="basetrade" ;
#CONFIGDIR="$HOME/$REPO/files/" ;
#CONFIGFILE="$CONFIGDIR/run_generate_ilists.config";
#CONFIGFILE="$HOME/modelling/gsq_store/`hostname -s`" ;
#if [ $ARG == "ALL" ] ;
#then
#    if [ -e $CONFIGFILE ];
#    then
#	SHC_list=`cat $CONFIGFILE | awk '{split($2,tk,"/");print tk[6];}' | uniq`;	
#    fi
#elif [ $ARG == "CONFIG" ] ;
#then
#    config=$
#else
#    SHC_list=$ARG;
#fi

if [ $config == "NA" ] ;
then
    for name in /home/dvctrader/modelling/indicatorwork/prod_configs/comb_config_$shc*_EU_MORN_DAY* ; do 
	if [ -e $name ]; then perl $HOME/basetrade_install/ModelScripts/generate_ilists.pl -un $USER -sc $shc -uf $name $common_string; fi ;  
    done	
    for name in /home/dvctrader/modelling/indicatorwork/prod_configs/comb_config_$shc*_US_MORN*; do 
	if [ -e $name ]; then perl $HOME/basetrade_install/ModelScripts/generate_ilists.pl -un $USER -sc $shc -uf $name $common_string; fi ; 
    done    
    for name in /home/dvctrader/modelling/indicatorwork/prod_configs/comb_config_$shc*_AS_MORN* ; do 
	if [ -e $name ]; then perl $HOME/basetrade_install/ModelScripts/generate_ilists.pl -un $USER -sc $shc -uf $name $common_string; fi ; 
    done    
    for name in /home/dvctrader/modelling/indicatorwork/prod_configs/comb_config_$shc*_AS_DAY* ; do 
	if [ -e $name ]; then perl $HOME/basetrade_install/ModelScripts/generate_ilists.pl -un $USER -sc $shc -uf $name $common_string; fi ; 
    done
else
    perl $HOME/basetrade_install/ModelScripts/generate_ilists.pl -un $USER -sc $shc -uf $config $common_string;
fi
