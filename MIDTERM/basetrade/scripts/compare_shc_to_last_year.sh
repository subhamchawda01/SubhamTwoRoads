#!/bin/bash

if [ $# -lt 1 ]; then
    echo "need one arg SHC";
    exit;
fi
SHC=$1; 
lnum=`/home/dvctrader/basetrade/scripts/pnl_analysis.sh $SHC 2015 | wc -l`; 
if [ $lnum -lt 1 ] ; then
    echo "no trading on this SHC in 2015";
    exit
fi

if [ $# -gt 1 ] ; then
    lnum=$((lnum-1)); 
fi

for YYYY in 2014 2015; do 
    /home/dvctrader/basetrade/scripts/pnl_analysis.sh $SHC $YYYY | head -n$lnum | tail -n1 ; 
done