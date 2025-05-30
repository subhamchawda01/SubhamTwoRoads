#!/bin/bash

USAGE="$0 PID YYYYMMDD ";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

PID=$1; shift;
YYYYMMDD=$1; shift;

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2};

REALLOGFILE=/apps/logs/QueryLogs/$NEWDIR/log.$YYYYMMDD.$PID.gz
REALTRADESFILE=/apps/logs/QueryTrades/$NEWDIR/trades.$YYYYMMDD.$PID

if [ -e $REALTRADESFILE ] ; then 
    if [ -e $REALLOGFILE ] ; then
	
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
	    
	    BASESIMID=`date +%N`; # ~ Random
	    DIVISOR=100000;
	    BASESIMID=`expr $BASESIMID / $DIVISOR `;
	    BASESIMID=$(( $BASESIMID + 1 ));
	    BASESIMID=`expr $BASESIMID \* $DIVISOR `;
		
		for MKT_MODEL in 0 1 2 3 4;  
		do
		    
		    SIMID=$(( $BASESIMID + $MKT_MODEL ));
		    
		    echo    $SIM_EXEC SIM $SIMSTRATFILENAME $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
		    $SIM_EXEC SIM $SIMSTRATFILENAME $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
		    rm -f /spare/local/logs/tradelogs/log.$YYYYMMDD.$SIMID

		    SIMTRADESFILE=/spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID
		    
		    SYMBOL=`head -1 $SIMTRADESFILE | awk '{print $3}' | cut -d . -f1`;
		    PREVID=`head -1 $SIMTRADESFILE | awk '{print $3}' | cut -d . -f2`;
		    
		    sed -e s#$SYMBOL.$PREVID#$SYMBOL.$SIMID# $SIMTRADESFILE > $SIMTRADESFILE"_1" ; 
		    
		    mv $SIMTRADESFILE"_1" $SIMTRADESFILE
		    
		    SIMTRADESFILELIST[$MKT_MODEL]=$SIMTRADESFILE;
		done
		
		~/infracore/scripts/plot_trades_pnl_nyc_2_all.sh $SIMTRADESFILE $REALTRADESFILE ${SIMTRADESFILELIST[@]}
		rm -f ${SIMTRADESFILELIST[@]} ;
	fi
    fi
fi