#!/bin/bash

if [ $# -lt 2 ]; then echo "USAGE: <script> inputfile outputfile"; exit 0; fi

ilist=$1;
output_ilist=$2
uid=`date +%N`;
tmpfile=tmp_"$uid";
head -n-1 $ilist | awk 'NR>3{print $_;}' > $tmpfile;
sort $tmpfile | uniq > "$tmpfile"_1; mv "$tmpfile"_1 $tmpfile;
head -n3 $ilist > $output_ilist;
cat $tmpfile >>  $output_ilist;
tail -n1 $ilist >>  $output_ilist;
