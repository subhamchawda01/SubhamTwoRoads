#!/bin/bash

printf "{\n";
printf "\t\"data\":[\n\n";

first=1 ;

for exch in `ls /spare/local/MDSlogs/zabbix/* | awk -F"/" '{print $NF}'` ; 
do

  if [ $first -ne 1 ] ; then 
    printf "\t,\n" ; 
  fi 

  first=0 ;
  printf "\t{\n";
  printf "\t\t\"{#MDSEXCH}\":\"$exch\"\n" ;
  printf "\t}\n";

done 

printf "\n\t]\n";
printf "}\n";
