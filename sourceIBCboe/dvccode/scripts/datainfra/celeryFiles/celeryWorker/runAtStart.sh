#!/bin/bash

#Configure aws
aws configure set aws_access_key_id AKIAJQ5XDKQW5X7ZG5EQ
aws configure set aws_secret_access_key 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
aws configure set default.region us-east-1

#Create logs folder
mkdir -p /media/disk-ephemeral2/results-logs/

#Change this
cp -r /mnt/sdf/dvccode/scripts/datainfra/celeryFiles ./

cd celeryFiles/celeryWorker
#Add cronjob
# crontab -l > tmp.txt; cat cronjob.txt >> tmp.txt; crontab tmp.txt

cd celeryScripts

#add queue name to config file
queue_name=`python support/getKey.py`

#echo "#celery queues" >> proj/celeryconfig.py
#echo CELERY_QUEUES = {\"$queue_name\": {\"exchange\": \"$queue_name\", \"routing_key\": \"$queue_name\"}} >> proj/celeryconfig.py 

#run celery
mkdir -p /media/disk-ephemeral2/celery_logs
source /etc/bashrc && source /home/dvctrader/.bashrc && celery worker -A proj -l info --config=proj.celeryconfig -Ofair &>/media/disk-ephemeral2/celery_logs/log.txt &
