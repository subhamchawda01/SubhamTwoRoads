#!/bin/bash

USAGE="$0 STRATFILEPATH YYYYMMDD [MKT_MODEL=0]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILEPATH=$1; shift;
YYYYMMDD=$1; shift;
MKT_MODEL=2;
if [ $# -gt 0 ] ; 
then
    MKT_MODEL=$1; shift;
fi


SIMID=`date +%N`; #SIMID=1711771;
SIMID=`expr $SIMID + 0`;

SIM_EXEC=~/basetrade_install/bin/sim_strategy
SIM_EXEC=~/LiveExec/bin/sim_strategy

if [ $USER == "sghosh" ] || [ $USER == "ravi" ] ;
then
    SIM_EXEC=~/basetrade_install/bin/sim_strategy
fi

if [ -e $STRATFILEPATH ] ; then
    SHORTCODE=`head -1 $STRATFILEPATH | awk '{print $2}'`;

    MKT_MODEL=`~/basetrade/scripts/get_market_model_for_shortcode.pl $SHORTCODE`;

    if [ $USER == "sghosh" ] || [ $USER == "ravi" ] ;
    then
	echo "$SIM_EXEC SIM $STRATFILEPATH $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1";
    fi

    $SIM_EXEC SIM $STRATFILEPATH $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
    rm -f /spare/local/logs/tradelogs/log.$YYYYMMDD.$SIMID

    SIMTRADESFILENAME=/spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID;
    if [ -e $SIMTRADESFILENAME ] ; then
	if [ $USER == "sghosh" ] || [ $USER == "ravi" ] ;
	then
	    ~/basetrade/scripts/plot_trades_pnl_nyc_2_all.sh $SIMTRADESFILENAME
	else
	    ~/basetrade/scripts/plot_trades_pnl_nyc_2_A.sh $SIMTRADESFILENAME
	fi
    fi
    rm -f $SIMTRADESFILENAME;
fi