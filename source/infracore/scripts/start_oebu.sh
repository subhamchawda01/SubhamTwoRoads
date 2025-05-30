#!/bin/bash
LOCKFILE=$HOME/locks/oebu.lock
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

FILE="$HOME/oebu_out";

$HOME/LiveExec/bin/our_extended_bidask_mkt_book_util $HOME/infracore_install/files/mkt_sec.txt > $FILE

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
