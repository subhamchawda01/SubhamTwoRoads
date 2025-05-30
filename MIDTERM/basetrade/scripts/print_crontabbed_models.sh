#!/bin/bash
for stratfilepath in `crontab -l | grep start | awk '{print $9}'`; do awk '{print $4}' $stratfilepath ; done 
