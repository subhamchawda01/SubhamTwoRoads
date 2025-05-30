#!/bin/bash
LOCKFILE=$HOME/locks/seehkpnls.lock;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

if [ ! -e $LOCKFILE ] ; then
    touch $LOCKFILE;

    FILE="$HOME/trades/ALL_HK_TRADES";

    PNL_SCRIPT="$HOME/LiveExec/scripts/see_ors_pnl.pl";

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

	GUI_TRADES_FILE=$HOME/trades/gui_hk_trades.$YYYYMMDD;
	if [ -e $GUI_TRADES_FILE ] ; then 
	    cat $GUI_TRADES_FILE >> $FILE ;
	fi
	
    # scp -q dvcinfra@sdv-chi-srv11:/spare/local/ORSlogs/CME/HC0/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-chi-srv13:/spare/local/ORSlogs/CME/G52/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-chi-srv14:/spare/local/ORSlogs/CME/VD4/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

#HKEX
        rsync -avz  --quiet dvcinfra@10.152.224.146:/spare/local/ORSlogs/HKEX/FITII/trades.$YYYYMMDD   $HOME/trades/hk12.trades.$YYYYMMDD
        cat $HOME/trades/hk12.trades.$YYYYMMDD >> $FILE;

        rsync -avz  --quiet dvcinfra@10.152.224.145:/spare/local/ORSlogs/HKEX/FITII/trades.$YYYYMMDD   $HOME/trades/hk11.trades.$YYYYMMDD
        cat $HOME/trades/hk11.trades.$YYYYMMDD >> $FILE;

##    rsync -avz  --quiet dvcinfra@10.220.40.1:/spare/local/ORSlogs/BMFEP/XLIN68/trades.$YYYYMMDD   $HOME/trades/bmf13.trades.$YYYYMMDD    
##    cat $HOME/trades/bmf13.trades.$YYYYMMDD >> $FILE;

	clear;
	perl -w $PNL_SCRIPT 'C' $FILE

	sleep 5;
    done

    rm -f $LOCKFILE;
else
    echo "$LOCKFILE present. Please delete";
fi
