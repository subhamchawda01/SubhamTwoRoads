#!/bin/bash

YYYYMMDD=`date +%Y%m%d`;

USAGE="$0 [YYYYMMDD=TODAY]";
if [ $# -ge 1 ] ; 
then 
    YYYYMMDD=$1; shift;
fi

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}

QUERYTRADEDIR=/NAS1/logs/QueryTrades;
for name in $QUERYTRADEDIR/$NEWDIR/trades.$YYYYMMDD.* ; 
do 
    PID=`echo $name | awk -F\. '{print $3}'`; 
    echo $PID ;
    MKT_MODEL=2;
    ~/infracore/scripts/sim_real_and_plot.sh $PID $YYYYMMDD $MKT_MODEL; 
done
