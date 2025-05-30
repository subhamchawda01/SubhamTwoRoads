#!/bin/bash

if [ $# -lt 1 ] ; then
    echo "need date";
    exit;
fi
DATE=$1; shift;

if [ $# -lt 1 ] ; then
    echo "need shc";
    exit;
fi
SHC=$1; shift;


SPLIT_DATE=`echo $DATE | sed 's/\(....\)\(..\)\(..\)/\1\/\2\/\3/'`;
fname="/NAS1/logs/QueryLogs/"$SPLIT_DATE"/log."$DATE".*.gz";

echo `zgrep STRATEGYLINE $fname 2>/dev/null | grep $SHC" " | awk '{ print \$9}' 2>/dev/null `;
