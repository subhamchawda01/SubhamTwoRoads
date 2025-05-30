#!/bin/bash

start_date=$1
end_date=$2
#i=$start_date
if [ $start_date -lt $end_date ] 
then
  for (( s = $start_date ; s<= $end_date ; ))
  do 
    dow=`date +%u -d $s`
    if [ $dow -lt "6" ]
    then 
      echo $s
    fi
    s=`date +%Y%m%d -d $s' +1 day'`
  done
else 
  for (( s = $start_date ; s>= $end_date ; ))
  do 
    dow=`date +%u -d $s`
    if [ $dow -lt "6" ]
    then 
      echo $s
    fi
    s=`date +%Y%m%d -d $s' -1 day'`
  done
fi

#date -d $start_date' - 1day '
