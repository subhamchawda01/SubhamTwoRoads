#!/bin/bash

d=`date +'%s'` ; 

~/controller_scripts/watch_running_only.sh  | sed 's/>.*//' | sort  -nk 2 | 
while read l ; 
do 
  #echo $l
  t=`echo $l | awk '{print $2}'` ;
  ts=`date -d @$t` ;
  echo -n $ts" " ;
  echo $l ;
done  ;
