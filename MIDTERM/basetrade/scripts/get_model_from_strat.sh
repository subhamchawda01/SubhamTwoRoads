#!/bin/bash

if [ $# -lt 1 ]
then 
    echo $0 stratfilename
    exit
fi

if [ -e $1 ]
then 
    f=$1
else
    f=`find ~/modelling/ -name $1`
fi
if [ -z "$f" ]; then echo "Could not find $1"; exit; fi;

echo '==>  STRATFILE <=== '$f
cat $f
echo ''
modelfl=`cut -d' ' -f 4 $f`
paramfl=`cut -d' ' -f 5 $f`


if [ $# -eq 1 ]
then 
    echo '====>  '$modelfl'  <==== (Size: '$(grep "INDICATOR " $modelfl | wc -l)
    cat $modelfl # | awk -v a=-2 '{print a, $0; a++; }'
    
    echo '====>  '$paramfl' <===='
    cat $paramfl
else
    cat $modelfl | head -n-1 | tail -n +4
fi
