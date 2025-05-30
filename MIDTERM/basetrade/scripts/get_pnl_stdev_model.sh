#!/bin/bash

if [ $# -gt 1 ] ; then
	YYYYMMDD=$1 
    STRAT_ID=$2
else
    echo "USAGE: YYYYMMDD STRAT_ID";
    exit;
fi

# Attempts to read the log file and created a temp model file.
# reads corresponding trades file, finds pnl, start time and end time of trading.
# finds std dev of model based during the start and end times

EXEC_DIR="/home/dvctrader/basetrade_install/"
Q_LOGS="/NAS1/logs/QueryLogs/"
T_LOGS="/NAS1/logs/QueryTrades/"

split_yymmdd=`echo $YYYYMMDD | sed 's/\(....\)\(..\)\(..\)/\1\/\2\/\3/'` 


Q_LOGS=$Q_LOGS""$split_yymmdd"/log."$YYYYMMDD"."$STRAT_ID".gz"
T_LOGS=$T_LOGS""$split_yymmdd"/trades."$YYYYMMDD"."$STRAT_ID



if ls $Q_LOGS $T_LOGS &> /dev/null; then
	#check if num trades >0
	NUM_LINES=`wc -l $T_LOGS | awk '{print $1}' `
	if [ "$NUM_LINES" -eq "0" ] ; then
		exit;
	fi
		
   	zcat $Q_LOGS | awk '/MODELINIT DEPBASE/,/INDICATOREND/' > /tmp/ilist_tmp
   	ST_HHMM=`head -1 $T_LOGS | awk '{printf("%d\n",$1)}' | xargs $EXEC_DIR"/bin/time_converter" FROM_SECS `
   	ET_HHMM=`tail -1 $T_LOGS | awk '{printf("%d\n",$1)}' | xargs $EXEC_DIR"/bin/time_converter" FROM_SECS `
   	CMD=$EXEC_DIR"/ModelScripts/get_stdev_model.pl /tmp/ilist_tmp "$YYYYMMDD" "$YYYYMMDD" "$ST_HHMM" "$ET_HHMM
   	#$echo "Executing: "$CMD
   	STDEV=` $CMD | head -n1 | awk '{print $1}'`
   	MODEL_PNL=`tail -1 $T_LOGS | awk '{print $9 " "$3}' | sed 's/\..*//'` #pnl and instrument
   	echo $YYYYMMDD" "$ST_HHMM"-"$ET_HHMM" "$STRAT_ID" "$STDEV" "$MODEL_PNL
   	rm /tmp/ilist_tmp
else
    echo "files do not exist, one or both: "$Q_LOGS" "$T_LOGS
fi
