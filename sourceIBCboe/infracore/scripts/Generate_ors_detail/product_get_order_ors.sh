#!/bin/bash

Data_Dir="/NAS1/data/ORSData/NSE/2019/10/"
Mds_Log="/tmp/mds_log_reader"
OutFile="/tmp/details_fileOct"
>$OutFile


declare -a sym=("IGL" )

for file in $Data_Dir*; do
   for i in ${sym[@]}; do
         d="$(basename -- $file)"
         date_="201910$d"
         echo $file
         tradcount=`$Mds_Log ORS_REPLY "/NAS1/data/ORSData/NSE/2019/10/$d/NSE_${i}_${date_}.gz" | grep "Exec" | wc -l`
         totcount=`$Mds_Log ORS_REPLY "/NAS1/data/ORSData/NSE/2019/10/$d/NSE_${i}_${date_}.gz" | egrep '\bConf\b|\bCxRe\b|\bCxlRejc\b|\bCxReRejc\b|\bCxld\b'|wc -l`
         echo "$i $date_ $totcount $tradcount"
         echo "$i $date_ $totcount $tradcount" >>$OutFile
     done
done


