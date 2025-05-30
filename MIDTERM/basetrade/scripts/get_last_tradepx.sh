#!/bin/bash

USAGE="$0 shc [YYYYMMDD=TODAY]";

if [ $# -lt 1 ] ;
then
    echo $USAGE;    
    exit;    
fi

shc=$1; shift;
YYYYMMDD="TODAY";

if [ $# -gt 0 ] ; then YYYYMMDD=$1; shift; fi

if [ $YYYYMMDD = "TODAY" ] ; then
  YYYYMMDD=`date +%Y%m%d`;            
fi
            
if [ $YYYYMMDD = "TODAY-1" ] ; then              
  YYYYMMDD=`date --date=yesterday +%Y%m%d`;
fi

last_working_date=$YYYYMMDD;
last_traded_px_=-1.0;

while [ `echo "$last_traded_px_<0.0" | bc` -eq 1 ] && [ $last_working_date -gt 20120101  ]; do
  last_working_date=`$HOME/basetrade_install/bin/calc_prev_week_day $last_working_date`;
  last_traded_px_=`$HOME/basetrade_install/bin/mds_log_l1_trade $shc $last_working_date | grep TRADE | tail -n1 | awk '{print $4}'`;
done
  
echo $last_traded_px_;
