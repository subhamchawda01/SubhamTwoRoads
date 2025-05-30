#!/bin/bash

printf "{\n";
printf "\t\"data\":[\n\n";

first=1 ;

for queryid in `ps -ef |grep -w tradeinit | grep -v "grep" | awk '{print $NF}' | sort -n | uniq`
do 

  product=`ps -ef |grep -w "$queryid" | grep -v grep | awk -F"LIVE" '{print $2}' | awk '{print $1}' | awk -F"/" '{print $6}' | tr -cd '[[:alnum:]]._-'` ;

  if [ $first -ne 1 ] ; then 
    printf "\t,\n" ; 
  fi 

  first=0 ;
  printf "\t{\n";
  printf "\t\t\"{#PRODUCT}\":\"$product\",\n";
  printf "\t\t\"{#QUERYID}\":\"$queryid\"\n" ;
  printf "\t}\n";

done 

printf "\n\t]\n";
printf "}\n";
