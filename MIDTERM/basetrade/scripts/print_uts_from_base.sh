#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi;

STRATFILENAME=$1; shift;

paramfile=`~/basetrade/scripts/print_param_from_base.sh $STRATFILENAME`;

if [ -s "$paramfile" ] ; then
    awk '{if($2=="UNIT_TRADE_SIZE"){print $3;}}' $paramfile;
fi;
