#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;

FULLSTRATPATH=~/modelling/strats/*/*/$STRATFILENAME;
echo $FULLSTRATPATH
if [ -e $FULLSTRATPATH ] ; then
    orig_name=`$HOME/basetrade/scripts/printOrigName.pl $FULLSTRATPATH`;
    orig_param=`$HOME/basetrade/scripts/printParam.pl $FULLSTRATPATH DEFAULT`;
    dir_path=`dirname $FULLSTRATPATH`;
    `awk -vopf=$orig_param '{$5=opf; print $_;}' $FULLSTRATPATH > $dir_path/$orig_name`;

    NEW_FULL_STRAT_PATH=`echo $FULLSTRATPATH | replace "modelling/strats" "modelling/pruned_strategies"`;

    DIR=`dirname $NEW_FULL_STRAT_PATH`;
    mkdir -p $DIR;

    echo ORIGSTRAT $dir_path/$orig_name;
    echo mv $FULLSTRATPATH $NEW_FULL_STRAT_PATH;
    mv $FULLSTRATPATH $NEW_FULL_STRAT_PATH;
fi
