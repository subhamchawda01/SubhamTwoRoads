#!/bin/bash

USAGE="$0 PID YYYYMMDD [MKT_MODEL=-1]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

PID=$1; shift;
YYYYMMDD=$1; shift;

MKT_MODEL=-1;
if [ $# -gt 0 ] ; 
then
    MKT_MODEL=$1; shift;
fi

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2};

SIMID=`date +%N`; # SIMID=1711771;
SIMID=`expr $SIMID + 0`;

REALLOGFILE=/apps/logs/QueryLogs/$NEWDIR/log.$YYYYMMDD.$PID.gz
REALTRADESFILE=/apps/logs/QueryTrades/$NEWDIR/trades.$YYYYMMDD.$PID

if [ -e $REALTRADESFILE ] ; then 

    SWORDCOUNT=`less -f $REALLOGFILE | head -1 | awk '{print NF}'`;
    if [ $SWORDCOUNT -ne 1 ] ; then
	exit;
    fi
    
    STRATFILEFULLNAME=`less -f $REALLOGFILE | head -1`;
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
    if [ $SIMSTRATFILENAME ]; then
    if [ -e $SIMSTRATFILENAME ] ; 
    then

	REALSYMBOL=`head -1 $SIMSTRATFILENAME | awk '{print $2}' `;
	
	if [ $MKT_MODEL -lt 0 ] ; then
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
		FDAX*)
		    MKT_MODEL=3;
		    ;;
		*)
		    MKT_MODEL=2;
		    ;;
	    esac
	fi
	

	SIM_EXEC=~/basetrade_install/bin/sim_strategy ;
#	SIM_EXEC=~/LiveExec/bin/sim_strategy ;
	
	echo    $SIM_EXEC SIM $SIMSTRATFILENAME $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
	$SIM_EXEC SIM $SIMSTRATFILENAME $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
	rm -f /spare/local/logs/tradelogs/log.$YYYYMMDD.$SIMID

	SIMTRADESFILE=/spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID
	if [ -e $SIMTRADESFILE ] ; then
	
	    SYMBOL=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f1`;
	    PREVID=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f2`;
	    
	    sed -e s#$SYMBOL.$PREVID#$SYMBOL.$SIMID# $SIMTRADESFILE > $SIMTRADESFILE"_1" ; 
	    mv $SIMTRADESFILE"_1" $SIMTRADESFILE
	    
	    ~/basetrade/ModelScripts/get_pnl_stats.pl $REALTRADESFILE 1;
	    ~/basetrade/ModelScripts/get_pnl_stats.pl $SIMTRADESFILE 1;
	    rm -f $SIMTRADESFILE
	fi
    fi
    fi
fi
