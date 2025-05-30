#!/bin/bash

if [ $# -gt 0 ] ; then
	YYYYMMDD=$1 
else
    echo "USAGE: YYYYMMDD";
    exit;
fi

# find all model_ids from the logs copied in /apps/logs/QueryLogs/
# calls get_pnl_stdev_model.sh for each strat_id for that day. stores the results in a result file

EXEC_DIR="/home/dvctrader/basetrade_install/"
Q_LOGS="/apps/logs/QueryLogs/"
T_LOGS="/apps/logs/QueryTrades/"
RES_FILE="/home/dvctrader/tmp/result_stdev_pnl.txt"

split_yymmdd=`echo $YYYYMMDD | sed 's/\(....\)\(..\)\(..\)/\1\/\2\/\3/'` 

CMD="ls  "$Q_LOGS""$split_yymmdd"/log."$YYYYMMDD".*.gz" 
for i in `$CMD | awk -F "." '{print $(NF-1) }' `
do
	CMD=$EXEC_DIR"scripts/get_pnl_stdev_model.sh "$YYYYMMDD" "$i 
	`$CMD >> $RES_FILE`
done;