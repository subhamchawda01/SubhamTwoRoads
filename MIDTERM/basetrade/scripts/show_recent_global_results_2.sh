#!/bin/bash

WF_DIR=$HOME/basetrade/walkforward/;

USAGE="$0 SHC STRATFILENAME HCOUNT ";
if [ $# -lt 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHC=$1; shift;
STRATFILENAME=$1; shift;
HCOUNT=$1; shift

GETEXEC_PY=$HOME/basetrade/scripts/get_exec_or_script.py

PRINT_STRAT_FROM_BASE_EXEC=`$GETEXEC_PY -e print_strat_from_base.sh`

STRATPATH=`$PRINT_STRAT_FROM_BASE_EXEC $STRATFILENAME 2>/dev/null`;

ISVALIDCONFIG_PY=`$GETEXEC_PY -s is_valid_config.py`

CONFIGID=`$ISVALIDCONFIG_PY -c $STRATFILENAME`;

TODAY=`date  +%Y%m%d`;

CALC_PREV_WEEK_DAY_EXEC=`$GETEXEC_PY -e calc_prev_week_day`

SUMMARIZE_STRAT_EXEC=`$GETEXEC_PY -e summarize_single_strategy_results`

PREVDAY=`$CALC_PREV_WEEK_DAY_EXEC $TODAY $HCOUNT`;


if [ $CONFIGID -gt 0 ]
then
  $SUMMARIZE_STRAT_EXEC $SHC $STRATFILENAME DB $PREVDAY $TODAY | tail -n$HCOUNT
elif [[ -s $STRATPATH ]]
then
  $SUMMARIZE_STRAT_EXEC $SHC $STRATFILENAME DB $PREVDAY $TODAY | tail -n$HCOUNT
else
  echo "No such config or strat...check name"
fi
