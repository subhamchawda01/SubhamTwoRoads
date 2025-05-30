#!/bin/bash 

dt=`date +%Y%m%d`;
hr=`date +%H`;

if [ $hr -gt 22 ]; then

  dt=`date --date='+1 day'` ;

fi ;

cat /spare/local/logs/tradelogs/trades.$dt'.'$2 2>/dev/null | awk '{print $6} END { if (!NR) print 0.0 }' | sort -n | head -1 | awk '{print $1*1.02}' ;
