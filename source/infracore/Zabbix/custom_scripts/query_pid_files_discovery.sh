#!/bin/bash

printf "{\n";
printf "\t\"data\":[\n\n";

first=1 ;

for querypid in `cat /spare/local/logs/tradelogs/PID_TEXEC_DIR/*` 
do 

  if [ $first -ne 1 ] ; then 
    printf "\t,\n" ; 
  fi 

  first=0 ;
  printf "\t{\n";
  printf "\t\t\"{#QUERYPID}\":\"$querypid\"\n" ;
  printf "\t}\n";

done 

printf "\n\t]\n";
printf "}\n";
