#!/bin/bash

addFile="/spare/local/files/NSE/action_to_consider.txt"

print_usage_and_exit (){
    echo "$0 YYYYMMDD DATA..."
    exit ;
}

if [ $# -lt 2 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

echo "$@" >> $addFile

tail -2 $addFile
