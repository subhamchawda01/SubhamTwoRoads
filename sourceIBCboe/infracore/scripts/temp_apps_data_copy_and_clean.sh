#!/bin/bash

#USAGE: $0 DATE=TODAY DAYS_TO_KEEP=2
dt="TODAY";
if [ $# -ge 1 ] ; then dt=$1; shift; fi;
if [ $dt == "TODAY" ] ; then dt=`date +%Y%m%d`; fi;
days=2;
if [ $# -ge 1 ] ; then days=$1; shift; fi

clear_dt=`$HOME/infracore_install/bin/calc_prev_week_day $dt $days`;
clear_yyyy=${clear_dt:0:4};
clear_mm=${clear_dt:4:2};
clear_dd=${clear_dt:6:2};

for y in $(seq 2011 $(expr $clear_yyyy - 1)); do rm -rf /apps/data/*Data/*/$y ; done
for m in $(seq 1 $(expr $clear_mm - 1)); do if [ $m -lt 10 ] ; then m="0"$m; fi; rm -rf /apps/data/*Data/*/$clear_yyyy/$m ; done
for d in $(seq 1 $clear_dd); do  if [ $d -lt 10 ] ; then d="0"$d; fi; rm -rf /apps/data/*Data/*/$clear_yyyy/$clear_mm/$d ; done

yyyy=${dt:0:4};
mm=${dt:4:2};
dd=${dt:6:2};

rsync -ravzm --include="*Data/*/$yyyy/$mm/$dd/*" --include="*/" --exclude='*' 10.23.74.52:/apps/data/ /apps/data/ ;


