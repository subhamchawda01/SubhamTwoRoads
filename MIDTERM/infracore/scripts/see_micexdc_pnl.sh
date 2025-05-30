#!/bin/bash
LOCKFILE=$HOME/locks/seemicexdc.lock
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

FILE="$HOME/trades/ALL_MICEXDC_TRADES";
PNL_SCRIPT="$HOME/LiveExec/scripts/see_ors_pnl.pl";

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


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

#    scp -q dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/ZLIN12/trades.$YYYYMMDD $HOME/trades/bmfdc_trades.$YYYYMMDD
#    cat $HOME/trades/bmfdc_trades.$YYYYMMDD >> $FILE; 

#    > $HOME/trades/bmfdc_trades.$YYYYMMDD;

    scp -q dvcinfra@172.18.244.107:/spare/local/ORSlogs/MICEXDC/MICEXDC/trades.$YYYYMMDD $HOME/trades/micexdc_trades.$YYYYMMDD
    cat $HOME/trades/micexdc_trades.$YYYYMMDD >> $FILE; 

    > $HOME/trades/micexdc_trades.$YYYYMMDD;

    clear;
    perl -w $PNL_SCRIPT 'C' $FILE
    sleep 30;
done

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
