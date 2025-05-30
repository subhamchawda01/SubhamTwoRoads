#!/bin/bash

USAGE="$0 PID YYYYMMDD [MKT_MODEL=0]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

PID=$1; shift;
YYYYMMDD=$1; shift;

if [ $# -gt 0 ] ; 
then
    MKT_MODEL=$1; shift;
else
    MKT_MODEL=2;
fi

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2};

SIMID=`date +%N`; # SIMID=1711771;
SIMID=`expr $SIMID + 0`;

REALLOGFILE=/apps/logs/QueryLogs/$NEWDIR/log.$YYYYMMDD.$PID.gz
REALTRADESFILE=/apps/logs/QueryTrades/$NEWDIR/trades.$YYYYMMDD.$PID

if [ -e $REALTRADESFILE ] ; then 

    REALSYMBOL=`head -1 $REALTRADESFILE | awk '{print $3}' | cut -d . -f1`;
    
    case $REALSYMBOL
	in
	ZT*)
	    MKT_MODEL=1;
	    ;;
	ZF*)
	    MKT_MODEL=1;
	    ;;
	ZN*)
	    MKT_MODEL=1;
	    ;;
	ZB*)
	    MKT_MODEL=2;
	    ;;
	FGBS*)
	    MKT_MODEL=3;
	    ;;
	FGBM*)
	    MKT_MODEL=3;
	    ;;
	FGBL*)
	    MKT_MODEL=3;
	    ;;
	FESX*)
	    MKT_MODEL=3;
	    ;;
	*)
	    MKT_MODEL=2;
	    ;;
    esac


    SWORDCOUNT=`less $REALLOGFILE | head -1 | awk '{print NF}'`;
    if [ $SWORDCOUNT -ne 1 ] ; then
	exit;
    fi
    
    STRATFILEFULLNAME=`less $REALLOGFILE | head -1`;
    STRATFILENAME=`basename $STRATFILEFULLNAME`;
    
    CUTCOUNT=5;
    CSFILE=`echo $STRATFILENAME | rev | cut -c$CUTCOUNT- | rev`;
    SIMSTRATFILENAME=`ls ~/modelling/strats/*/*/$CSFILE 2>/dev/null`;
    while [ $? -gt 0 ] ; 
    do
	CUTCOUNT=$((CUTCOUNT+1));
	if [ $CUTCOUNT -gt 10 ] ;
	then
	    break;
	fi
	
	CSFILE=`echo $STRATFILENAME | rev | cut -c$CUTCOUNT- | rev`;
	SIMSTRATFILENAME=`ls ~/modelling/strats/*/*/$CSFILE 2>/dev/null`;
    done
    
    if [ -e $SIMSTRATFILENAME ] ; 
    then
	SIM_EXEC=~/infracore_install/bin/sim_strategy ;
#	SIM_EXEC=~/LiveExec/bin/sim_strategy ;
	
	echo    $SIM_EXEC SIM $SIMSTRATFILENAME $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
	$SIM_EXEC SIM $SIMSTRATFILENAME $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
	rm -f /spare/local/logs/tradelogs/log.$YYYYMMDD.$SIMID
	
	SYMBOL=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f1`;
	PREVID=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f2`;
	
	SIMTRADESFILE=/spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID
	sed -e s#$SYMBOL.$PREVID#$SYMBOL.$SIMID# $SIMTRADESFILE > $SIMTRADESFILE"_1" ; 
	
	mv $SIMTRADESFILE"_1" $SIMTRADESFILE
	
	~/infracore/scripts/plot_trades_pnl_nyc_2_all.sh $SIMTRADESFILE $REALTRADESFILE
	rm -f $SIMTRADESFILE
    fi
fi
