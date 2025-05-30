#!/bin/bash
LOCKFILE=$HOME/locks/seetmxatr.lock
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

FILE="$HOME/trades/ALL_TMXATR_TRADES";
PNL_SCRIPT="$HOME/infracore_install/scripts/see_tmx_atr_ors_pnl.pl";

if [ ! -d $HOME/trades ] ;
then
    mkdir $HOME/trades;
fi

while [ true ]
do
    > $FILE;

    YYYYMMDD=$(date "+%Y%m%d");
    
    if [ $# -eq 1 ] ;
    then
	YYYYMMDD=$1;
    fi

    #TMX server
    #/spare/local/ORSlogs/TMXATR/BDMATR/atr_trades.20111018
    scp -q dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMXATR/BDMATR/atr_trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/atr_trades.$YYYYMMDD >> $FILE; 
    > $HOME/trades/atr_trades.$YYYYMMDD;


#    scp -q dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMXATR/BDMATR/atr_trades.$YYYYMMDD $HOME/trades
#    cat $HOME/trades/atr_trades.$YYYYMMDD >> $FILE; 
#    > $HOME/trades/atr_trades.$YYYYMMDD;


    clear;
    perl -w $PNL_SCRIPT $FILE

    sleep 30;
done

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
