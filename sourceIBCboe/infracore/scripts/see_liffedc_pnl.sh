#!/bin/bash
LOCKFILE=$HOME/locks/seeliffedc.lock
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

FILE="$HOME/trades/ALL_LIFFEDC_TRADES";
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

    scp -q dvcinfra@10.23.52.52:/spare/local/ORSlogs/LIFFE/EJG9/trades.$YYYYMMDD $HOME/trades/liffedc_trades.$YYYYMMDD
    cat $HOME/trades/liffedc_trades.$YYYYMMDD >> $FILE; 

    scp -q dvcinfra@10.23.52.53:/spare/local/ORSlogs/LIFFE/JG9/trades.$YYYYMMDD $HOME/trades/liffedc_trades.$YYYYMMDD
    cat $HOME/trades/liffedc_trades.$YYYYMMDD >> $FILE; 

    > $HOME/trades/liffedc_trades.$YYYYMMDD;

    clear;
    perl -w $PNL_SCRIPT 'C' $FILE

    sleep 30;
done

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
