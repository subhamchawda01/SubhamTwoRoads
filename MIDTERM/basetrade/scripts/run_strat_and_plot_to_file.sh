#!/bin/bash

USAGE="$0 STRATFILENAME YYYYMMDD [DATA_START_TIME=taken_from_pick_strat_config]";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;
YYYYMMDD=$1; shift;
DATA_START_TIME=-1;
if [ $# -gt 0 ] ; 
then
    DATA_START_TIME=$1; shift;
fi


SIMID=`date +%N`; # SIMID=1711771;
SIMID=`expr $SIMID + 0`;

SIM_EXEC=$HOME/LiveExec/bin/sim_strategy
if [ ! -e $SIM_EXEC ] ; then
    SIM_EXEC=$HOME/basetrade_install/bin/sim_strategy
fi

WF_DIR=$HOME/basetrade/walkforward/

FULLSTRATFILEPATH=$STRATFILENAME;
if [ ! -e $FULLSTRATFILEPATH ] ; then
    FULLSTRATFILEPATH=$HOME/modelling/*strats/*/*/$STRATFILENAME;
fi

BASENAME=`basename $FULLSTRATFILEPATH`;
MKT_MODEL=1
TEMPSTRATNAME="";

if [ -e $FULLSTRATFILEPATH ] ; then
    CONFIGID=`$WF_DIR/is_valid_config.py -c $BASENAME`
    
    if [ $CONFIGID -gt 0 ]; then
      TEMPSTRATNAME="/tmp/temp_strat_$SIMID";
      $WF_DIR/print_strat_from_config.py -c $BASENAME -d $YYYYMMDD > $TEMPSTRATNAME;
      cat $TEMPSTRATNAME;  
      FULLSTRATFILEPATH=$TEMPSTRATNAME;
    else
      SHORTCODE=`head -1 $FULLSTRATFILEPATH  | awk '{print $2}'`;
      MKT_MODEL=`$HOME/basetrade/scripts/get_market_model_for_shortcode.pl $SHORTCODE`;
    fi

    if [ `echo "$DATA_START_TIME < 0" | bc` -gt 0 ] ; then
      DATA_START_TIME=`$HOME/basetrade/scripts/get_data_start_time_for_strat.pl $FULLSTRATFILEPATH $YYYYMMDD`;
      if [ `echo "$DATA_START_TIME < 0" | bc` -gt 0 ] ; then $DATA_START_TIME="0.0"; fi
    fi 

    echo $SIM_EXEC SIM $FULLSTRATFILEPATH $SIMID $YYYYMMDD $MKT_MODEL 0 $DATA_START_TIME 0 ADD_DBG_CODE -1

    $SIM_EXEC SIM $FULLSTRATFILEPATH $SIMID $YYYYMMDD $MKT_MODEL 0 $DATA_START_TIME 0 ADD_DBG_CODE -1
    rm -f /spare/local/logs/tradelogs/log.$YYYYMMDD.$SIMID

    SYMBOL=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f1`;
    PREVID=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f2`;

    sed -i s/$SYMBOL.$PREVID/$SYMBOL.$YYYYMMDD/g /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID

    sort /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID -o /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID
    GRAPHFILE=/spare/local/logs/tradelogs/graph.$YYYYMMDD.$SIMID
    if [ -e $GRAPHFILE ] ; then rm -f $GRAPHFILE ; fi 
    $HOME/basetrade/scripts/plot_trades_pnl_nyc_2.pl /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID F $GRAPHFILE
    rm -f /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID

    if [ -e $TEMPSTRATNAME ]; then
      rm -f $TEMPSTRATNAME;
    fi
fi
