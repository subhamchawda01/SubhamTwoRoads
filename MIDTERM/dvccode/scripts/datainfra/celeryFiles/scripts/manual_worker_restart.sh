ps aux | grep celery\ worker | awk '{print $2}' | xargs kill 
cd /home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryWorker/celeryScripts
#Check all process killed
cnt=`ps uax | grep celery\ worker | wc -l`
while [ $cnt -gt 1 ]; do
	sleep 5;
	cnt=`ps uax | grep celery\ worker | wc -l`
done
source /etc/bashrc && source /home/dvctrader/.bashrc && celery worker -A proj -l info --config=proj.celeryconfig -Ofair &> /media/disk-ephemeral2/celery_log.txt &
source /etc/bashrc && source /home/dvctrader/.bashrc && celery worker -A proj -l info --config=proj.slow_celeryconfig -Ofair -n slow.%h &> /media/disk-ephemeral2/celery_log_slow.txt &
