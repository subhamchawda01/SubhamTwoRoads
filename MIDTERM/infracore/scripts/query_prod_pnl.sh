#!/bin/bash

USAGE="$0 PROD SDATE EDATE ";
if [ $# -lt 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

PROD=$1; shift;
SDATE=$1; shift;
EDATE=$1; shift;

~/infracore/scripts/query_pnl_data.pl $PROD $SDATE $EDATE | sort | awk -F"," '{ sum = sum + $3 ; print $1,$3,sum,$4 }'
