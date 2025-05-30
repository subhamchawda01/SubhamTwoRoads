#!/bin/bash

USAGE="$0 EPOCHTIME";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

REPO=infracore;
GENSCRIPTSDIR=$HOME/$REPO/scripts;

EPOCHTIME=$1; shift;

env TZ=Europe/Berlin $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME