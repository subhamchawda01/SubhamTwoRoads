#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;

ls ~/LiveModels/strats/*/*/$STRATFILENAME"_"*

