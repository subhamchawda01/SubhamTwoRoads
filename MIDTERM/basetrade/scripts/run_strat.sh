#!/bin/bash

USAGE="$0 STRATFILENAME YYYYMMDD";
if [ $# -lt 2 ] ;
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;
YYYYMMDD=$1; shift;

SIM_EXEC=~/cvquant_install/basetrade/bin/sim_strategy

if [ $# -gt 0 ];
then
    SIM_EXEC=$1; shift;
fi

STRUCTURED=0
if [ $# -gt 0 ];
then
    STRUCTURED=$1; shift;
fi

SIMID=`date +%N`; # SIMID=1711771;
SIMID=`expr $SIMID + 0`;

FULLSTRATFILEPATH=$STRATFILENAME;
if [ ! -e $FULLSTRATFILEPATH ] ; then FULLSTRATFILEPATH=~/modelling/*strats/*/*/$STRATFILENAME; fi
if [ ! -e $FULLSTRATFILEPATH ] ; then FULLSTRATFILEPATH=`ls /apps/logs/QueryLogs/*/*/*/$STRATFILENAME | head -n1`; fi

WF_DIR=$HOME/basetrade/walkforward/;
BASENAME=`basename $STRATFILENAME`;
MKT_MODEL=1
TEMPSTRATNAME="";

CONFIGID=`$WF_DIR/is_valid_config.py -c $BASENAME`

if [ $CONFIGID -gt 0 ]; then
  TEMPSTRATNAME="/tmp/temp_strat_$SIMID";
  $WF_DIR/print_strat_from_config.py -c $BASENAME -d $YYYYMMDD -s $STRUCTURED > $TEMPSTRATNAME;
  cat $TEMPSTRATNAME;
  FULLSTRATFILEPATH=$TEMPSTRATNAME;
else
  SHORTCODE=`head -1 $FULLSTRATFILEPATH  | awk '{print $2}'`;
  MKT_MODEL=`~/basetrade/scripts/get_market_model_for_shortcode.pl $SHORTCODE`;
fi

DATA_START_TIME=`$HOME/basetrade/scripts/get_data_start_time_for_strat.pl $FULLSTRATFILEPATH $YYYYMMDD`;
if [ `echo "$DATA_START_TIME < 0" | bc` -gt 0 ] ; then $DATA_START_TIME="0.0"; fi


echo "$SIM_EXEC SIM $FULLSTRATFILEPATH $SIMID $YYYYMMDD $MKT_MODEL 0 $DATA_START_TIME 0 ADD_DBG_CODE -1";
$SIM_EXEC SIM $FULLSTRATFILEPATH $SIMID $YYYYMMDD $MKT_MODEL 0 $DATA_START_TIME 0 ADD_DBG_CODE -1

SYMBOL=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f1`;
PREVID=`head -1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID | awk '{print $3}' | cut -d . -f2`;

sed -e s#$SYMBOL.$PREVID#$SYMBOL.$SIMID# /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID > /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID_1 ;

mv /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID_1 /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID

echo /spare/local/logs/tradelogs/trades.$YYYYMMDD.$SIMID

if [ -e $TEMPSTRATNAME ]; then
    rm -f $TEMPSTRATNAME;
fi
