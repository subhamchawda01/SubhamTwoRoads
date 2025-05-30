#!/bin/bash
if [ $# -eq 1 ] ; then
    for strat_file in `crontab -l | grep start_real | awk '{ print $9}' `; do cat $strat_file | grep $1 | awk '{ print $9}';done 
else
    crontab -l | grep start | awk '{print $9}'
fi
