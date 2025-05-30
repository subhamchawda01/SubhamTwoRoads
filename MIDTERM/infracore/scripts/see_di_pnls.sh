#!/bin/bash
LOCKFILE=$HOME/locks/seedipnls.lock;
if [ ! -e $LOCKFILE ] ; then
    touch $LOCKFILE;

    FILE="$HOME/trades/DI_TRADES";

    trade_date_=$(date "+%Y%m%d");

    PNL_SCRIPT="$HOME/infracore_install/scripts/see_di_pnls.pl";

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

    # Brazil
	rsync -avz  --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/XALP0038/trades.$YYYYMMDD   $HOME/trades/bmf11_di.trades.$YYYYMMDD    
	cat $HOME/trades/bmf11_di.trades.$YYYYMMDD >> $FILE;

	clear;
	perl -w $PNL_SCRIPT $FILE

	sleep 10;
    done

    rm -f $LOCKFILE;
else
    echo "$LOCKFILE present. Please delete";
fi
