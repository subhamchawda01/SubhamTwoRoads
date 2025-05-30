#!/bin/bash

USAGE="$0 LOGFILENAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


LOGFILENAME=$1; shift;
for name in `grep Starting $LOGFILENAME | awk '{print $NF}'`; do ~/infracore/scripts/unixtime2nytime.sh $name ; done | uniq -c
