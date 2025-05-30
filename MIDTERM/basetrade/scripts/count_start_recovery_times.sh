#!/bin/bash

USAGE="$0 LOGFILENAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi


LOGFILENAME=$1; shift;
for name in `grep Starting $LOGFILENAME | awk '{print $NF}'`; do ~/basetrade/scripts/unixtime2nytime.sh $name ; done | uniq -c