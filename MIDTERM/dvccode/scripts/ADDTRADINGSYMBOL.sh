#!/bin/bash

slack_exec=/home/pengine/prod/live_execs/send_slack_notification 
addts_config_file=/home/pengine/prod/live_configs/`hostname`"_"addts.cfg

if [ $# -ge 1 ] ; then 
  addts_config_file=$1 ;

  if [ ! -f $addts_config_file ] ; then 
    echo "ADDTS-FAILURE -> CAN'T READ $addts_config_file" ; 
    exit ;
  fi  

fi

[ -f $addts_config_file -a -s $addts_config_file -a -r $addts_config_file ] || `$slack_exec "prod-issues" "DATA" "ADDTS-FAILURE -> MISSING $addts_config_file"` ;

while read line 
do
	if [[  $( echo $line | head -c 1 ) != '#'  &&  $(echo "$line" | wc -w) -ge 7  ]] ;
	then
		/home/pengine/prod/live_scripts/ors_control.pl $line ;
                sleep 1 ;
	fi 
done < $addts_config_file
