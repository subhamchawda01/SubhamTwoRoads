#!/bin/bash

USAGE1="$0 SHORTCODE"
EXAMP1="$0 BR_DOL_0"

if [ $# -ne 1 ] ; 
then 
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

SHC=$1;

for name in $HOME/modelling/strats/$SHC/*/* ; 
do 
    bname=`basename $name`; 
    numoc=`ls -altr $HOME/modelling/strats/$SHC/*/$bname | wc -l` ; 
    if [ $numoc -gt 1 ] ; then 
	echo $bname ; 
    fi ; 
done
