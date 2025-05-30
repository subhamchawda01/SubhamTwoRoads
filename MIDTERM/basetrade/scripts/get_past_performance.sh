#!/bin/bash

if [ $# -lt 1 ]
then
    echo $0 \<strategy file name\>
    exit
fi
f=`find ~/modelling/ -name $1`
id=`cat $f | awk '{print $NF}'`
id=${id::2}
echo $id
for i in /NAS1/logs/QueryLogs/2012/0[56789]/*/log.*.$id*.gz
do
    a=`zcat $i | head -n4 | grep $1`
    printf "%s\r" $i
    if [ ! -z $a ]; then 
	f=`echo $i | sed 's/QueryLogs/QueryTrades/; s/log\./trades\./; s/\.gz//'`
	printf "%s "  $i
	tail -n1 $f
    fi
done
echo '---------------------------Complete-----------------------------'