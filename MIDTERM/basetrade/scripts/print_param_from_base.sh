#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;

STRATPATH=`$HOME/basetrade/scripts/print_strat_from_base.sh $STRATFILENAME 2>/dev/null`;

if [[ -s $STRATPATH ]]; then $HOME/basetrade/scripts/printParam.pl $STRATPATH;
else (>&2 echo "No Strat found"); exit 2; fi;

