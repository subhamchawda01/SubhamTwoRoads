#!/bin/bash
APPEND_FILE="/tmp/check_addts_count"

today_date=`date +\%Y\%m\%d`
[[ `ps aux | grep cme | grep -v "grep" | wc -l` < "1" ]] && { echo "Ors Not Running"; exit; }
ORS_DIR=`ps aux | grep cme | grep -v "grep" | awk 'BEGIN{FS=" ";} {print $15}'`
LOG_FILE="$ORS_DIR/log.$today_date"
line_no=`grep -nar "Opened ORSLog in Append" $LOG_FILE | tail -1 | cut -d':' -f1`
tail -n +$line_no $LOG_FILE >$APPEND_FILE
log_count=`grep -a "ADDTRADING" $APPEND_FILE|wc -l`
echo "$log_count"
