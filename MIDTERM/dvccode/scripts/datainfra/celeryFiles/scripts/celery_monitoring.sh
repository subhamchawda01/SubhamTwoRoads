#Check after 5 mins that command has executed successfully

SSH_VARS="-o StrictHostKeyChecking=no  -o ConnectTimeout=20"
CMD="sleep 1"
#CMD="sleep 10"
output=`run_my_job.py -n dvctrader -m 1 -c "$CMD"`
url=`echo $output | grep http`
id=`echo $url | awk -F'/' '{print $NF}' | awk -F' ' '{print $1}'`

#sleep for 5 mins
sleep 120
state=`view_job_status.py -i $id | grep STATE | awk -F' ' '{print $NF}'`
if [ "$state" != "SUCCESS" ]; then 
	echo "Process not successful since 5 mins. Process state: $state" > /spare/local/trash/celery_tmp_check
	echo "== SERVER STATUS ==" >> /spare/local/trash/celery_tmp_check
	ssh $SSH_VARS ec2-user@52.91.139.132 '/home/ec2-user/show_status.sh' >> /spare/local/trash/celery_tmp_check
	/home/dvctrader/infracore_install/bin/send_slack_notification result_issues FILE /spare/local/trash/celery_tmp_check
fi
