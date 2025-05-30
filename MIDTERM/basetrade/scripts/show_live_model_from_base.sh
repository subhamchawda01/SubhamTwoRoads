#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;

STRATPATH=`$HOME/basetrade/scripts/print_livestrat_file_from_base.sh $STRATFILENAME 2>/dev/null`;

if [[ $file ]]; then ~/basetrade/scripts/showModel.pl $STRATPATH; 
else (>&2 echo "No Strat found"); exit 2; fi;
