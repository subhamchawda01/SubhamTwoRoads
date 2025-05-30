#!/bin/bash

# Remove all temporary files created by see_ors_pnl.pl (for storing symbol->remaining days map) before exitting
trap 'rm -rf  $HOME/.DI_remaining_days.*' EXIT

PRINT_PNL_SCRIPT="$HOME/infracore/scripts/print_pnl.sh"
LOCKFILE=$HOME/locks/seeallpnls.lock;
if [ ! -e $LOCKFILE ] ; then
    touch $LOCKFILE;

    while [ true ]
    do
		$PRINT_PNL_SCRIPT 'C'
		sleep 5;
    done

    rm -f $LOCKFILE;
else
    date;
    ls -lrt $LOCKFILE;
    echo "$LOCKFILE present. Please delete";
fi
