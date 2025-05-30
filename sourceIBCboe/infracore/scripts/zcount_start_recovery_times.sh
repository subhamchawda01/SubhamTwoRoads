#!/bin/bash

USAGE="$0 LOGFILENAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi


LOGFILENAME=$1; shift;
for name in `zgrep Starting $LOGFILENAME | awk '{print $NF}'`; do ~/infracore/scripts/unixtime2nytime.sh $name ; done | uniq -c