#!/bin/bash

USAGE="$0 EXCH SDATE EDATE ";
if [ $# -lt 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

EXCH=$1; shift;
SDATE=$1; shift;
EDATE=$1; shift;

~/infracore/scripts/query_pnl_data.pl $EXCH $SDATE $EDATE | sort | awk -F"," '{ sum = sum + $3 ; printf "%d %d %d %d %4.2f\n",$1,$3,sum,$4,($3/$4); }'
