#!/bin/bash


USAGE="$0 SHORTCODE POOLSTARTTIME POOLENDTIME";
if [[ ( $# -lt 3  ) ]] ; 
then 
    echo $USAGE;
    exit;
fi

SHORTCODE=$1;
POOLSTARTTIME=$2;
POOLENDTIME=$3;

#use home variable later
for files in `ls "/home/dvctrader/modelling/wf_strats/"$SHORTCODE"/"$POOLSTARTTIME"-"$POOLENDTIME`; do
     echo $files;
done;
