#!/bin/bash



sec=`ps aux | grep cme | grep -v grep | awk '{print $9}' | awk -F: '{ print ($1 * 3600) + ($2 * 60) +60 }'`; 


#echo $sec
ps aux | grep trade_engine | grep -v grep | awk '{print $9,$12,$16}' | tr ':' ' ' | awk -v sc="$sec" '{time=$1":"$2} {if(($1 * 3600) + ($2 * 60) <= sc) print time,$3,$4}'

