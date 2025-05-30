#!/bin/bash 

dt=`date +%Y%m%d`;
hr=`date +%H`;

if [ $hr -gt 22 ]; then

  dt=`date --date='+1 day'` ;

fi ;

tail -n1 /spare/local/logs/tradelogs/trades.$dt'.'$2 2>/dev/null | awk '{print $9} END { if (!NR) print 0.0 }'
