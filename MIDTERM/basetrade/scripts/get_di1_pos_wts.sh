#!/bin/bash

USAGE="$0 shc_list [YYYYMMDD=TODAY] [base_shc=DI1F16]";

if [ $# -lt 1 ] ;
then
    echo $USAGE;    
    exit;    
fi

shc_list=$1; shift;
YYYYMMDD="TODAY";
base_shc="DI1F16";

if [ $# -gt 0 ] ; then YYYYMMDD=$1; shift; fi
if [ $# -gt 0 ] ; then base_shc=$1; shift; fi

if [ $YYYYMMDD = "TODAY" ] ; then
  YYYYMMDD=`date +%Y%m%d`;            
fi
            
if [ $YYYYMMDD = "TODAY-1" ] ; then              
  YYYYMMDD=`date --date=yesterday +%Y%m%d`;
fi
  
base_dv01=`$HOME/basetrade_install/scripts/get_di1_straing_dv01.sh $base_shc $YYYYMMDD`;
for shc in `cat $shc_list`; do 
  dv01=`$HOME/basetrade_install/scripts/get_di1_straing_dv01.sh $shc $YYYYMMDD`;
  echo $shc `echo "scale=5; $dv01/$base_dv01" | bc`;
done
