#!/bin/bash
if [ $# -lt 3 ];
then
	echo "USAGE:<script><strattegyfile><date><output_directory>";
	exit;
fi

STRATFILE=$1;
DATE=$2;
OUTPUT_DIR=$3;
SIMEXEC="$HOME/LiveExec/bin/sim_strategy";
DATAGENEXEC="$HOME/LiveExec/bin/datagen";
SENDTRADEFILE="$OUTPUT_DIR/tmp_sendtrade_file";
ORDEREXECFILE="$OUTPUT_DIR/tmp_orderexec_file";
MODELFILE=`awk '{print $4}' $STRATFILE`;
STARTTIME=`awk '{print $6}' $STRATFILE`;
ENDTIME=`awk '{print $7}' $STRATFILE`;
pid=`echo $((1 + RANDOM % 1000))`;
echo `"$SIMEXEC" "SIM" "$STRATFILE" "$pid" "$DATE" "ADD_DBG_CODE" "PLSMM_INFO" 2>/dev/null`
`grep "SendOrderExch" /spare/local/logs/tradelogs/log."$DATE".$pid | awk '{for(i=1;i<=NF;i++){if($i ~ /CAOS/){print $1,$(i+1)}}}' | sort -n -u -k2,2 > $SENDTRADEFILE`;
`grep "ExecutedOrder" /spare/local/logs/tradelogs/log."$DATE".$pid | awk '{for(i=1;i<=NF;i++){if($i ~ /CAOS/){print $1,$(i+1)}}}' | sort -n -u -k2,2 > $ORDEREXECFILE`;

