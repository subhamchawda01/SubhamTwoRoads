#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;

file=`ls ~/LiveModels/*strats/*/*/$STRATFILENAME 2>/dev/null`;
if [[ ! $file ]]; then
  file=`ls ~/LiveModels/*strats/*/EBT/*/$STRATFILENAME 2>/dev/null`;
fi;

if [[ $file ]]; then echo $file; 
else (>&2 echo "No Strat found"); exit 2; fi;
