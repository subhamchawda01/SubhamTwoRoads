#!/bin/bash

USAGE="$0 CONFIGFILE [num_days=252]";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

name=$1; shift;
days=252;
if [ $# -gt 0 ]; then days=$1; shift; fi;

file=`echo -n "$name" | awk '{n=split($1, tks, "/"); print tks[n] }'`;

mkdir -p $HOME/locks ;
LOCKFILE=$HOME/locks/gen_ind_$file".lock";

if [ ! -e $LOCKFILE ] ;
then
    touch $LOCKFILE;
    if [ -e $name ] ; 
    then 
	$HOME/basetrade_install/ModelScripts/generate_multiple_indicator_stats_2_distributed.pl $name $days; 
    else
	echo "config file $name doesnt exists\n";
    fi
    rm -f $LOCKFILE;
else
    echo "$LOCKFILE present, check if the number of processes running are more than config files in the queue file on this server";
fi
