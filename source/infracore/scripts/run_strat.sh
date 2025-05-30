#!/bin/bash

USAGE="$0 STRATFILENAME YYYYMMDD";
if [ $# -ne 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;
YYYYMMDD=$1; shift;

SIMID=`date +%N`; # SIMID=1711771;
SIMID=`expr $SIMID + 0`;

~/infracore_install/bin/sim_strategy SIM ~/modelling/strats/*/*/$STRATFILENAME $SIMID $YYYYMMDD ADD_DBG_CODE -1

SYMBOL=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f1`;
PREVID=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f2`;

sed -e s#$SYMBOL.$PREVID#$SYMBOL.$SIMID# /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID > /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID_1 ; 

mv /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID_1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID

echo /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID
