BASE_PATH='/media/shared/ephemeral16/slowCeleryCmnds/'
test "$(ls -A "$BASE_PATH" 2>/dev/null)" || exit 0;

TOTAL=`find /media/shared/ephemeral16/slowCeleryCmnds/ | xargs wc -l | grep total | awk -F' ' '{print $1}'`
SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
CURR_JOBS=`ssh $SSH_VARS ec2-user@52.91.139.132 "sudo /usr/sbin/rabbitmqctl list_queues -p vhostClient name messages | grep -w slow" | awk -F' ' '{print $NF}'`
OVERALL_MAX=25
if [ $CURR_JOBS -ge $OVERALL_MAX ]; then
	exit 0;
fi
MAX_SCHEDULE=$(expr $OVERALL_MAX - $CURR_JOBS)
MAX=$(( $MAX_SCHEDULE > $TOTAL ? $TOTAL:$MAX_SCHEDULE))
for file in `find $BASE_PATH -type f`; do
	total=`wc -l  < $file`
	ans=$total
	if [ $total -gt 5 ]; then
		ans=`echo "$total * $MAX / $TOTAL" | bc -l`
		ans=`echo "$ans/1" | bc`
	fi
	head -n$ans $file > /spare/local/logs/slow_celery_tmp_file
	if [ $ans -eq $total ]; then
		rm $file;
	else
		tail -n +$(expr $ans + 1) $file > /spare/local/logs/tmp_slow
		mv /spare/local/logs/tmp_slow $file
	fi

	echo "`date`, File: $file, Num Cmnds: $ans" >> /spare/local/logs/celery_scheduler_slow
	/home/dvctrader/celeryFiles/celeryClient/celeryScripts/run_my_job.py -f /spare/local/logs/slow_celery_tmp_file -m 1 -n dvctrader -s 1 -q slow >>  /spare/local/logs/celery_scheduler_slow
done
