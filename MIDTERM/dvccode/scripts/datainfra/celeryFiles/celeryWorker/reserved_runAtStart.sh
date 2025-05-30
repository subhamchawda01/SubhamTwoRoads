#!/bin/bash
cp -r /mnt/sdf/dvccode/scripts/datainfra/celeryFiles /home/dvctrader/
cd /home/dvctrader/celeryFiles/celeryWorker/celeryScripts
#Start fast queue
source /etc/bashrc && source /home/dvctrader/.bashrc && celery worker -A proj -l info --config=proj.celeryconfig -Ofair &> /media/disk-ephemeral2/celery_log.txt &
#Start Slow queue
source /etc/bashrc && source /home/dvctrader/.bashrc && celery worker -A proj -l info --config=proj.slow_celeryconfig -Ofair -n slow.%h &> /media/disk-ephemeral2/celery_log_slow.txt &
#Start Local Jobs queue
source /etc/bashrc && source /home/dvctrader/.bashrc && celery worker -A proj -l info --config=proj.local_celeryconfig -Ofair -n local.%h &> /media/disk-ephemeral2/celery_log_local &
