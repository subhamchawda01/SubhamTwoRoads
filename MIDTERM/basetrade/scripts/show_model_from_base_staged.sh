#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;

~/basetrade/scripts/showModel.pl ~/modelling/*staged_strats/*/*/$STRATFILENAME

echo "";
