#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;

~/infracore/scripts/showParam.pl ~/modelling/strats/*/*/$STRATFILENAME

