#!/bin/bash

if [ $# -lt 2 ];
then
	echo "USAGE:<script><strattegyfile><date>";
	exit;
fi

STRATFILE=$1;
DATE=$2;
SIMEXEC="$HOME/LiveExec/bin/sim_strategy";
DATAGENEXEC="$HOME/LiveExec/bin/datagen";
CORRSCRIPT="$HOME/basetrade/ModelScripts/indicator_corr_and_class_rate.py";
SENDTRADEFILE="tmp_sendtrade_file";
ORDEREXECFILE="tmp_orderexec_file";
DATAGENOUTPUTFILE="tmp_datagen_file";
MODELFILE=`awk '{print $4}' $STRATFILE`;
STARTTIME=`awk '{print $6}' $STRATFILE`;
ENDTIME=`awk '{print $7}' $STRATFILE`;

echo `"$SIMEXEC" "SIM" "$STRATFILE" "9021344" "$DATE" "ADD_DBG_CODE" "PLSMM_INFO" 2>/dev/null`
`grep "SendOrderExch" /spare/local/logs/tradelogs/log."$DATE".9021344 | awk '{for(i=1;i<=NF;i++){if($i ~ /CAOS/){print $1,$(i+1)}}}' | sort -n -u -k2,2 > $SENDTRADEFILE`;
`grep "ExecutedOrder" /spare/local/logs/tradelogs/log."$DATE".9021344 | awk '{for(i=1;i<=NF;i++){if($i ~ /CAOS/){print $1,$(i+1)}}}' | sort -n -u -k2,2 > $ORDEREXECFILE`;
`"$DATAGENEXEC" "$MODELFILE" "$DATE" "$STARTTIME" "$ENDTIME" "32897" "$DATAGENOUTPUTFILE" "1000" "c1" "0" "0"`;
echo "`"$CORRSCRIPT" "$MODELFILE" "$DATAGENOUTPUTFILE" "$SENDTRADEFILE" "$ORDEREXECFILE"`"; 

rm -rf $SENDTRADEFILE;
rm -rf $DATAGENOUTPUTFILE;
rm -rf $ORDEREXECFILE;
