#!/bin/bash

USAGE="$0 SHC ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHC=$1; shift;

mkdir -p $HOME/locks ;
LOCKFILE=$HOME/locks/gen_comb_ind_stats_prod_$SHC".lock";
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

hname=`hostname -s`;

case $hname in 
    sdv-ny4-srv13)

	for name in $HOME/indicatorwork/prod_configs/comb_config_$SHC*_EU_MORN_DAY*[tdx] ; do 
	    if [ -e $name ] ; then $HOME/basetrade_install/ModelScripts/generate_multiple_indicator_stats_2_distributed.pl $name ; fi ;  
	done

	;;
    sdv-ny4-srv12)

	for name in $HOME/indicatorwork/prod_configs/comb_config_$SHC*_US_MORN_DAY*[x] ; do 
	    if [ -e $name ]; then $HOME/basetrade_install/ModelScripts/generate_multiple_indicator_stats_2_distributed.pl $name ; fi ; 
	done
	
	for name in $HOME/indicatorwork/prod_configs/comb_config_$SHC*_AS_MORN*[x] ; do 
	    if [ -e $name ]; then $HOME/basetrade_install/ModelScripts/generate_multiple_indicator_stats_2_distributed.pl $name ; fi ; 
	done
	
	for name in $HOME/indicatorwork/prod_configs/comb_config_$SHC*_AS_DAY*[x] ; do 
	    if [ -e $name ]; then $HOME/basetrade_install/ModelScripts/generate_multiple_indicator_stats_2_distributed.pl $name ; fi ; 
	done

	;;
esac

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
