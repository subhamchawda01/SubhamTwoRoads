ps uax | grep flower\ -A\ proj | awk -F' ' '{print $2}' | xargs kill -9
cd /home/ec2-user/celeryFiles/celeryWorker/celeryScripts/ && /usr/local/bin/flower -A proj --config=proj.celeryconfig --persistent  --max_workers=100 --max_tasks=30000 --port=5555 &>>/tmp/celeryLogs/flowercron_log.txt &
