#!/bin/bash

USAGE="$0 STRATFILENAME DATE[OPTIONAL ARG ONLY FOR WALKFORWARD]";
if [[ ( $# -ne 1 ) && ( $# -ne 2 ) ]]; then
  echo $USAGE;
  exit;
fi
                
STRATFILENAME=$1;

GETEXEC_PY=$HOME/basetrade/scripts/get_exec_or_script.py

PRINT_STRAT_FROM_BASE_EXEC=`$GETEXEC_PY -e print_strat_from_base.sh`

STRATPATH=`$PRINT_STRAT_FROM_BASE_EXEC $STRATFILENAME 2>/dev/null`;

ISVALIDCONFIG_PY=`$GETEXEC_PY -s is_valid_config.py`

PRINTSTRATFROMCONFIG_PY=`$GETEXEC_PY -s print_strat_from_config.py`

PROCESSCONFIG_PY=`$GETEXEC_PY -s process_config.py`

SHOWMODEL_PL=`$GETEXEC_PY -s showModel.pl`

CONFIGID=`$ISVALIDCONFIG_PY -c $STRATFILENAME`

if [ $CONFIGID -gt 0 ]; then
  if [ $# -eq 2 ]; then
    STRAT=`$PRINTSTRATFROMCONFIG_PY -c $STRATFILENAME -d $2 2>/dev/null`;
    PARAM=`echo $STRAT | awk '{print $5}'`;
    if [[ -f "$PARAM" ]]; then cat $PARAM; fi
  else
    $PROCESSCONFIG_PY -c $STRATFILENAME -m VIEWPARAM; 
  fi;
else
  if [[ -s $STRATPATH ]]; then
    $SHOWMODEL_PL $STRATPATH;
  else
    (>&2 echo "No Strat or Config found");
    exit 2;
  fi;
fi;

