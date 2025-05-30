#!/bin/bash
LOCKFILE=$HOME/locks/seehkdcpnls.lock;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

if [ ! -e $LOCKFILE ] ; then
    touch $LOCKFILE;

    FILE="$HOME/trades/ALL_HK_DC_TRADES";

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

	GUI_TRADES_FILE=$HOME/trades/gui_hk_dc_trades.$YYYYMMDD;
	if [ -e $GUI_TRADES_FILE ] ; then 
	    cat $GUI_TRADES_FILE >> $FILE ;
	fi

#HKEX
        rsync -avz  --quiet dvcinfra@10.152.224.145:/spare/local/ORSlogs/HKEX/FITIIDC/trades_bd4.$YYYYMMDD   $HOME/trades/hk11_dc.trades.$YYYYMMDD
        cat $HOME/trades/hk11_dc.trades.$YYYYMMDD >> $FILE;


	clear;
	perl -w $PNL_SCRIPT 'C' $FILE

	sleep 5;
    done

    rm -f $LOCKFILE;
else
    echo "$LOCKFILE present. Please delete";
fi
