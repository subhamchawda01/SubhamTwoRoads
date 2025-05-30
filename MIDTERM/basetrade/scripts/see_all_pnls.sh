#!/bin/bash
LOCKFILE=$HOME/locks/seeallpnls.lock;
if [ -e $LOCKFILE ] ; then

FILE="$HOME/trades/ALL_TRADES";
PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_pnl.pl";

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

    if [ -e $HOME/trades/gui_trades ] ; then 
	cat $HOME/trades/gui_trades >> $FILE ;
    fi
    
    scp -q dvcinfra@sdv-chi-srv11:/spare/local/ORSlogs/CME/HC0/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

#    scp -q dvcinfra@sdv-chi-srv12:/spare/local/ORSlogs/CME/J55/trades.$YYYYMMDD $HOME/trades
#    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    scp -q dvcinfra@sdv-chi-srv13:/spare/local/ORSlogs/CME/G52/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    scp -q dvcinfra@sdv-chi-srv14:/spare/local/ORSlogs/CME/VD4/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    #EUREX server
    scp -q dvcinfra@sdv-fr2-srv11:/spare/local/ORSlogs/EUREX/UTE002/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    scp -q dvcinfra@sdv-fr2-srv14:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    scp -q dvcinfra@sdv-fr2-srv13:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    #TMX server
    scp -q dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/BDMA/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    scp -q dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/BDMB/trades.$YYYYMMDD $HOME/trades
    cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;


    clear;
    perl -w $PNL_SCRIPT $FILE

    sleep 5;
done

else
echo "$LOCKFILE present. Please delete";
fi
