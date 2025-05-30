#!/bin/bash

printf "{\n";
printf "\t\"data\":[\n\n";

first=1 ;

for mdsdata in `ls /spare/local/MDSlogs/zabbix/* | grep -v "GENERIC_GENERIC" | awk -F"/" '{print $NF}'`
do 

  if [ $first -ne 1 ] ; then 
    printf "\t,\n" ; 
  fi 

  first=0 ;
  printf "\t{\n";
  printf "\t\t\"{#MDSDATA}\":\"$mdsdata\"\n" ;
  printf "\t}\n";

done 

printf "\n\t]\n";
printf "}\n";
