#!/bin/bash

SCRIPT="$HOME/infracore/scripts/print_pnl.sh"
COMPUTE_SCRIPT="$HOME/infracore/scripts/compute_pnl.sh"
FORMAT="script mode [YYYYMMDD] [EOD](dump_EOD_position and store EOD pnls)"
if [ $# -lt 1 ] ;
then
    echo $FORMAT;
    exit 0;
fi

MODE=$1;
if [[ $MODE -ne 'C' && $MODE -ne 'R' && $MODE -ne 'E' ]] 
then
	echo "MODE has to be C/R/E. Exiting..."
	exit 0;
fi

GENERAL_OUT_FILE_C="$HOME/PrintPnl/pnls_now_c"
GENERAL_OUT_FILE_R="$HOME/PrintPnl/pnls_now_r"
GENERAL_OUT_FILE_E="$HOME/PrintPnl/pnls_now_e"
OUT_PRODUCED="$HOME/PrintPnl/out_produced"
EOD_CHECK_FILE="$HOME/PrintPnl/eod_check_file"

if [ $# -ge 3 ] ;
then
    EOD=$3;
	if [ $EOD -eq 1 ]
	then
		touch $EOD_CHECK_FILE
	fi
fi

CHECK_IF_RUNNING=`ps -efH | grep compute_pnl.sh | grep "/bin/bash" | grep -v grep`

if [ ! -n "$CHECK_IF_RUNNING" ]
then
	$COMPUTE_SCRIPT $@
    
	if [ "$MODE" = "C" ] 
	then
		cat $GENERAL_OUT_FILE_C
	fi
	
	if [ "$MODE" = "R" ] 
	then
		cat $GENERAL_OUT_FILE_R
	fi
	
	if [ "$MODE" = "E" ] 
	then
		cat $GENERAL_OUT_FILE_E
	fi
	
else
	timeout=600;
	time_=0;
	sleep 1
	while [ ! -f $OUT_PRODUCED ]
	do
		CHECK_IF_RUNNING=`ps -efH | grep compute_pnl.sh | grep "/bin/bash" | grep -v grep`
		if [ ! -n "$CHECK_IF_RUNNING" ]
		then
			break;
		fi
		
		sleep 1
  		time_=$((time_+1));
  		if [ $time_ -eq $timeout ]
  		then
  			echo "Timeout"
  			exit 0;
		fi
		
	done
	
	if [ ! -f $OUT_PRODUCED ]
	then
		exec $SCRIPT $@
	fi
	
	if [ "$MODE" = "C" ] 
	then
		cat $GENERAL_OUT_FILE_C
	fi
	
	if [ "$MODE" = "R" ] 
	then
		cat $GENERAL_OUT_FILE_R
	fi
	
	if [ "$MODE" = "E" ] 
	then
		cat $GENERAL_OUT_FILE_E
	fi	
fi

sleep 2
exit 1;