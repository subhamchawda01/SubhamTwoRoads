#!/bin/bash

# Remove all temporary files created by see_ors_pnl.pl (for storing symbol->remaining days map) before exitting
trap 'rm -rf  $HOME/.DI_remaining_days.*' EXIT

LOCKFILE=$HOME/locks/seearbpnls.lock;
if [ ! -e $LOCKFILE ] ; then
    touch $LOCKFILE;

    FILE="$HOME/trades/ALL_ARB_TRADES";

    trade_date_=$(date "+%Y%m%d");
    CME_EU_VOL_INFO_FILE=$HOME/trades/cme_eu_vol_info_file_$trade_date_ ;

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

	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/MS005/trades.$YYYYMMDD   $HOME/trades/bmf11_arb.trades.$YYYYMMDD
	cat $HOME/trades/bmf11_arb.trades.$YYYYMMDD >> $FILE;
	
        clear;
	perl -w $PNL_SCRIPT 'C' $FILE $YYYMMDD 0 BMF 1 | grep -v INVALI

	sleep 5;
    done

    rm -f $LOCKFILE;
else
    echo "$LOCKFILE present. Please delete";
fi
