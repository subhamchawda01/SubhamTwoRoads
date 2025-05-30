#!/bin/bash

USAGE="$0 SHORTCODE recordfile DEPBASE DEPPRED SDATE EDATE";

if [ $# -lt 6 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHORTCODE=$1; shift;
recordfile=$1; shift;
DEPBASE=$1; shift;
DEPPRED=$1; shift;
SDATE=$1; shift;
EDATE=$1; shift;

~/basetrade/ModelScripts/get_indicator_stats_by_date.pl $recordfile $SDATE $EDATE | grep -v MEDIAN | awk 'BEGIN{ printf "MODELINIT DEPBASE '$SHORTCODE' '$DEPBASE' '$DEPPRED'\n"; printf "MODELMATH LINEAR CHANGE\n"; printf "INDICATORSTART\n"; } { printf "INDICATOR 1.00 "; for ( i = 4 ; i <= NF ; i ++ ) { printf "%s ", $i }; printf "# %.2f\n", $3 } END{ printf "INDICATOREND\n"; }'

