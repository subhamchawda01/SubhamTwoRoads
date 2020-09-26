#!/bin/bash

if [ $# ! -eq 1 ] ; then
  echo "USAGE: <script> <shortcode_position file>"
  exit
fi

DATE=`date +%Y%m%d`
UNDERBAN_ADDTS=$1
UNDERBAN_FILE="/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_${DATE}.csv"
ADDTS_FILE="/tmp/under_ban_addts_prod"
ORS_DIR=`ps aux | grep cme | grep -v grep | awk '{print $15}'`
PROFILER=`echo $ORS_DIR | cut -d'/' -f6`

for prod in `sed 's/_/ /g' $UNDERBAN_ADDTS | awk '{print $2}' | uniq`;
do
sed -i "/$prod/d" $UNDERBAN_FILE
done

/home/pengine/prod/live_scripts/ors_control.pl NSE $PROFILER RELOADMARGINFILE

awk -v prof="$PROFILER" '{print "NSE",prof,"ADDTRADINGSYMBOL",$1,$2,$2,$2,$2}' $UNDERBAN_ADDTS > $ADDTS_FILE

/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh $ADDTS_FILE
