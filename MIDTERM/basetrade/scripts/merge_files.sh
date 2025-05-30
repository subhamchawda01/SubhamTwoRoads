#!/bin/bash

if [ $# -lt 3 ] ; then echo "$0 newfile masterfile mergefile"; exit 0; fi ; 

newfile=$1
masterfile=$2
mergefile=$3

uid=`date +%N`
dir=/tmp/mergefiles/$uid
mkdir -p $dir

sort $newfile > $dir/a; 
sort $masterfile > $dir/b; 

cat $dir/a $dir/b | awk '{map[$1] = $_;} END{for (i in map) {print map[i]}}' | sort > $mergefile ; 
echo `diff $dir/a $dir/b | wc -l` `diff $mergefile $dir/b | wc -l` `diff $mergefile $dir/a | wc -l`; 
wc -l $newfile $masterfile $mergefile; 

rm -r $dir
