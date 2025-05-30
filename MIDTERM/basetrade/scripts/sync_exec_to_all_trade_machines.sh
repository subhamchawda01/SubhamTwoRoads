#!/bin/bash

USAGE="$0 TRD_EXEC";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRD_EXEC=$1; shift;

for TRD_MAC in 10.23.196.51 10.23.196.52 10.23.196.53 10.23.196.54 10.23.200.51 10.23.200.52 10.23.200.53 10.23.200.54 10.23.182.51 10.23.182.52 10.23.23.11 10.23.23.12 10.23.23.13 10.23.23.14 10.23.52.51 10.23.52.52 10.23.52.53 ;
do 
    echo $TRD_MAC
    ~/basetrade/scripts/sync_exec_to_trade_machine.sh $TRD_MAC $TRD_EXEC
done
