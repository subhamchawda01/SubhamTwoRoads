#!/bin/bash

dir="$HOME/modelling/stir_strats/"
common_prod=$1;

if [ $# -lt 1 ]; then echo "$USAGE <common_prod> <prod>"; 
elif [ $# -lt 2 ];
  then
    for i in $dir/$common_prod/*/* ; do base=`basename $i`; echo "$base" ; done
else
    prod=$2;
    for i in $dir/$common_prod/*/* ; do base=`basename $i`; echo "$base"_"$prod" ; done
fi
