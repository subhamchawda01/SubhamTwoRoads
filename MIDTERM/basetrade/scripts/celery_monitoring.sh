#Check after 5 mins that command has executed successfully

SSH_VARS="-o StrictHostKeyChecking=no  -o ConnectTimeout=20"
CMD="sleep 1"
#CMD="sleep 10"
output=`/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py -n dvctrader -m 1 -c "$CMD"`
url=`echo $output | grep http`
id=`echo $url | awk -F'/' '{print $NF}' | awk -F' ' '{print $1}'`

#sleep for 10 mins
sleep 600
state=`/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/view_job_status.py -i $id | grep STATE | awk -F' ' '{print $NF}'`
if [ "$state" != "SUCCESS" ]; then 
	echo "Process not successful since 10 mins. Process state: $state" > /spare/local/trash/celery_tmp_check
	echo "== SERVER STATUS ==" >> /spare/local/trash/celery_tmp_check
	ssh $SSH_VARS ec2-user@52.91.139.132 '/home/ec2-user/show_status.sh' >> /spare/local/trash/celery_tmp_check
#	/home/dvctrader/infracore_install/bin/send_slack_notification datainfra FILE /spare/local/trash/celery_tmp_check
fi
