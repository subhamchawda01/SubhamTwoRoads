#!/usr/bin/env bash

if [ $# -lt 1 ] ; then
    echo "need a result file";
    exit;
fi

grname=$1; shift;
if [ -e $grname ] ; then 
    $HOME/basetrade/scripts/remove_duplicates_globalresults_on_ec2.py $grname > $grname"_" ; 
    mv $grname"_" $grname ; 
fi
