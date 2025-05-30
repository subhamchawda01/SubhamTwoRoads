#!/bin/bash

USAGE="$0 config_file";
if [ $# -lt 1 ]
then
    echo $USAGE;
    exit;
fi

while read line 
do
	if [[  $( echo $line | head -c 1 ) != '#'  &&  $(echo "$line" | wc -w) -ge 7  ]] ;
	then
		/home/pengine/prod/live_scripts/ors_control.pl $line ;
                sleep 1 ;
	fi 
done < $1 
