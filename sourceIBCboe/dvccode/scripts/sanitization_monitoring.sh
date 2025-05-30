print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

YYYYMMDD=$1 ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}

combined_writer_prodlog_dir='/NAS1/logs/PRODServerLogs'
server_list_file='/home/pengine/prod/live_configs/monitor_sanitization_server_list.cfg'
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification

for server in `cat $server_list_file`
do
	logfile="$combined_writer_prodlog_dir/$server/$YYYY/$MM/$DD/combined_writer_log.$YYYYMMDD.gz"
	for product in `zcat $logfile | grep Cross | awk '{ print $4 }' | sort -u`
	do
		message=" Product : $product No of sanitizations: `zcat $logfile | grep Cross | grep $product | wc -l ` Date : $YYYYMMDD"
		$send_slack_exec alerts DATA "$message"
	done
done

