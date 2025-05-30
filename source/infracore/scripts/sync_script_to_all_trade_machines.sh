#!/bin/bash

USAGE="$0 SCRIPT_PATH_FROM_BASETRADE";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

SCRIPT_PATH=$1; shift;

for TRD_MAC in 10.23.196.51 10.23.196.52 10.23.196.53 10.23.196.54 10.23.200.51 10.23.200.52 10.23.200.53 10.23.200.54 10.23.182.51 10.23.182.52 10.23.23.11 10.23.23.12 10.220.40.1 ;
do 
    echo $TRD_MAC
    ~/infracore/scripts/sync_script_to_trade_machine.sh $TRD_MAC $SCRIPT_PATH
done
