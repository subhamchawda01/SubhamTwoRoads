#!/bin/bash

pythonScript="/home/pengine/prod/live_scripts/gsm_files_download.py"
declare -A urlListMap
urlListMap=([gsm-latest-]="https://www.nseindia.com/api/reportGSM?csv=true" [MW-NIFTY-50-]="https://www.nseindia.com/api/equity-stockIndices?csv=true&index=NIFTY%2050" [MW-NIFTY-NEXT-50-]="https://www.nseindia.com/api/equity-stockIndices?csv=true&index=NIFTY%20NEXT%2050")
destDir="/home/dvcinfra/important/GsmNiftyFiles"
#destDir=`pwd`  # for checking

dd=`date "+%d"`
mmm=`date "+%b"`
yyyy=`date "+%Y"`

for filename in "${!urlListMap[@]}";do
  i=0
  while  [ ! -f "$destDir/${filename}$dd-$mmm-$yyyy.csv" ];do
     echo "try $i"
     /apps/anaconda/anaconda3/bin/python3 "$pythonScript" "$filename" "${urlListMap[$filename]}"
     mv "$filename" "$destDir/${filename}$dd-$mmm-$yyyy.csv"
     #sleep 10s
     i=$(( $i + 1 ))
  done
  [ -f "$destDir/${filename}$dd-$mmm-$yyyy.csv" ] && echo "$destDir/${filename}$dd-$mmm-$yyyy.csv present"
done
