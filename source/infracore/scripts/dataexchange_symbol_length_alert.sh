#!/bin/bash

DATAEXCHANGE_SYMBOL_FILE="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt";

count=`awk '{if(length($1)>=14){present=1;}} END {if(present == 1) print "present"}' $DATAEXCHANGE_SYMBOL_FILE`

if [[ "$count" == "present" ]]; then 
  echo "present"; 
  echo "" | mail -s "DATAEXCHANGE_SYMBOL LENGTH >= 16" -r "${HOSTNAME}-${USER}<hardik.dhakate@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in , sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
fi
