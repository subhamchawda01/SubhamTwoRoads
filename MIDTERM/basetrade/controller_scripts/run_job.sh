sh /home/dvctrader/ec2_jobs/run_cmd_$job_id &
pid=$!

touch "executing_"$job_id

wait ${pid}
touch "done_"$job_id
#rm "executing_"$job_id
#rc=$!

