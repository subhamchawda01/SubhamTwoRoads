#!/bin/bash
for i in `cat $HOME/AWSScheduler/running_jobs | awk '{print $1}'`; do grep -R $i queues/ | head -1 ; done | awk '{print $1}' | cut -d':' -f1 | sort | uniq -c
