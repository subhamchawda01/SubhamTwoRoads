#!/bin/bash
if [ $# -lt 2 ] ; then echo "USAGE: $0 file1 file2"; exit ; fi

FILE1=$1;
FILE2=$2;

sort $FILE1 > ~/tmp_f1 ;
sort $FILE2 > ~/tmp_f2 ;

diff ~/tmp_f1 ~/tmp_f2 ;
rm -f ~/tmp_f1 ~/tmp_f2 ;
