#!/bin/bash

USAGE="$0 STRATFILENAME ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;
HCOUNT=15;
if [ $# -gt 0 ] ; 
then
    HCOUNT=$1; shift;
fi

grep -hw $STRATFILENAME /home/dvctrader/globalresults/*/2011/*/*/*.txt | awk '{ $1=""; print; }' | sort -g | tail -n$HCOUNT

