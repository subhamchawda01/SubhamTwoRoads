#!/bin/bash

if [[ $# -lt 1 ]];
then
  echo "USAGE $0 date"
  exit
fi;

today=$1

if [[ $(date -d"$today" +%w) == 1 ]]
then
    LOOK_BACK=3
else
    LOOK_BACK=1
fi

yesterday=`date -d "$today ${LOOK_BACK} day ago" +%Y%m%d`

yesterday_histfile_number=`ls \/spare\/\local\/MeanRevertPort\/HistFiles\/* | grep $yesterday | wc -l`
today_histfile_number=`ls \/spare\/\local\/MeanRevertPort\/HistFiles\/* | grep $today | wc -l`

if [ $yesterday_histfile_number -ne $today_histfile_number ]
then
    message="Number of Histfiles generated between yesterday and today are different.\n$today $today_histfile_number\n$yesterday $yesterday_histfile_number"
   `echo -e $message | mail -s "Histfile generation alert" mehul.goyal@tworoads.co.in hrishav.agarwal@tworoads.co.in kaushik.putta@circulumvite.com`
   exit 1
fi

