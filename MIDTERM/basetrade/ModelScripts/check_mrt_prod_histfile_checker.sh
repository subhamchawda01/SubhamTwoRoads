#!/bin/bash

if [[ $# -lt 1 ]];
then
  echo "USAGE $0 date"
  exit
fi;

date=$1
source /home/dvctrader/.bashrc
shopt -s expand_aliases
config="$HOME/dvccode/configs/mrt_query_server_map"
not_existing_histfiles=""
output_string=""
while IFS= read -r line;
do
  echo $line
  queryid=$(echo $line | awk '{print $1}')
  server=$(echo $line | awk '{print $2}')
  #echo $server
  strat_cmd="ssh$server -n \"crontab -l | grep $queryid | awk '{if ( NF > 12 ) print \\\$9}'\""
  echo $strat_cmd
  stratfile=$(eval $strat_cmd)
  if [[ "$stratfile" == "" ]]
  then
    echo "QueryId $queryid not found on server $server"
  fi
  #echo $stratfile
  paramfile_cmd="ssh$server -n \"cat $stratfile | awk '{print \\\$5}'\""
  echo $paramfile_cmd
  paramfile=$(eval $paramfile_cmd)
  #echo $paramfile
  histfile_cmd="ssh$server -n \"cat $paramfile | grep HISTFILE_PREFIX | awk '{print \\\$3}'\""
  echo $histfile_cmd
  histfile=$(eval $histfile_cmd)
  histfile=$histfile$date
  #echo $histfile
  cmd="test -f $histfile"
  if [[ $(eval "ssh$server -n -q $cmd") ]]
  then
    output_string=$output_string$server" "$histfile"\n";
  else
    echo "Histfile Found"
  fi;
done < $config;
if [[ "$output_string" != ""  ]]
then
  echo $output_string
  mail_cmd="printf \"$output_string\" | mail -s \"Check Histfile on Prod Server $date\" mehul.goyal@tworoads.co.in hrishav.agarwal@tworoads.co.in kaushik.putta@circulumvite.com"
  eval $mail_cmd
fi
