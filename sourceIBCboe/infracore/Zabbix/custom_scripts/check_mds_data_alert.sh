#!/bin/bash 

if [ -s /spare/local/MDSlogs/zabbix/$1 ] ; then 
  tail -n1 /spare/local/MDSlogs/zabbix/$1 ; 
else 
  echo "RECOVERY";
fi
