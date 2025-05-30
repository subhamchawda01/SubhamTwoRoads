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
  
last_traded_px_=`$HOME/basetrade_install/scripts/get_last_tradepx.sh $shc $YYYYMMDD`;
  
dv01=`$HOME/basetrade_install/bin/get_dv01_for_shortcode $shc $YYYYMMDD $last_traded_px_`
echo $dv01;
