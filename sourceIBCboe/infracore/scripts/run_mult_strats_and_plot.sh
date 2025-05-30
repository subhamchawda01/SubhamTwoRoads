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

SIM_EXEC=~/infracore_install/bin/sim_strategy
SIM_EXEC=~/LiveExec/bin/sim_strategy

if [ -e $STRATFILEPATH ] ; then
SHORTCODE=`head -1 $STRATFILEPATH | awk '{print $2}'`;

    case $SHORTCODE
	in
	ZT_0)
	    MKT_MODEL=1;
	    ;;
	ZF_0)
	    MKT_MODEL=1;
	    ;;
	ZN_0)
	    MKT_MODEL=1;
	    ;;
	ZB_0)
	    MKT_MODEL=2;
	    ;;
	FGBS_0)
	    MKT_MODEL=3;
	    ;;
	FGBM_0)
	    MKT_MODEL=3;
	    ;;
	FGBL_0)
	    MKT_MODEL=3;
	    ;;
	FESX_0)
	    MKT_MODEL=3;
	    ;;
	*)
	    MKT_MODEL=2;
	    ;;
    esac


$SIM_EXEC SIM $STRATFILEPATH $SIMID $YYYYMMDD $MKT_MODEL ADD_DBG_CODE -1
rm -f /spare/local/logs/tradelogs/log.$YYYYMMDD.$SIMID

SIMTRADESFILENAME=/spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID;
if [ -e $SIMTRADESFILENAME ] ; then
#~/infracore/scripts/plot_trades_pnl_nyc_2_all.sh $SIMTRADESFILENAME
    ~/infracore/scripts/plot_trades_pnl_nyc_2_A.sh $SIMTRADESFILENAME
fi
rm -f $SIMTRADESFILENAME;
fi