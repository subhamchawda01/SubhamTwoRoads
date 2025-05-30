#!/bin/bash
USAGE="$0 PROD IP"
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi
ssh $2 "bash -s" <  /home/dvctrader/basetrade/scripts/print_crontabbed_strats.sh $1

