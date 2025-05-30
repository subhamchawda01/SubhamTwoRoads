#!/bin/bash
for stratfilepath in `crontab -l | grep start | awk '{print $9}'`; do awk '{print $6}' $stratfilepath ; done 
