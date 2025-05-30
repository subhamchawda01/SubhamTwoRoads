#!/bin/bash

USAGE="$0 SHC ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHC=$1; shift;

mkdir -p $HOME/locks ;
LOCKFILE=$HOME/locks/gen_ind_stats_prod_$SHC".lock";
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

for name in $HOME/indicatorwork/prod_configs/prod_config_$SHC* ; do ~/basetrade/ModelScripts/generate_indicator_stats.pl $name ; done

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
