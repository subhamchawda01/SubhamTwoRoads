#!/bin/bash

USAGE="$0 STOCKNAME ";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STOCKNAME=$1; shift;

FULLSTRATFILEPATH=$STOCKNAME;

for STRAT_NAME in $HOME/modelling/stir_strats/*/*/*;
do
  IM_STRAT=`awk '{ print \$2 }' $STRAT_NAME`;
  if [ -e $IM_STRAT ] ; then 
    STOCK_STRAT_STRING=`grep $STOCKNAME $IM_STRAT`;
    if [ X"$STOCK_STRAT_STRING" != "X" ] ; then 
    echo $STRAT_NAME; 
    fi
  fi
done 

