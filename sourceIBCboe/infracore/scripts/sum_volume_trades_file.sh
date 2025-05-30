#!/bin/bash

USAGE="$0 TRADESFILENAME ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRADESFILENAME=$1; shift;

if [ -e $TRADESFILENAME ] ; then
#1318434413.952197 FLAT FGBM201112.3011 B    2 120.8700000    0        0      323 [    24 120.870000 X 120.880000   247 ]
    awk '{ sum += $5; } END{print sum;}' $TRADESFILENAME
fi
