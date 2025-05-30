for worker in 10.0.1.45 10.0.1.47 10.0.1.178 10.0.1.68; do 
	ssh -n -f dvctrader@$worker '/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/scripts/manual_worker_restart.sh'
done

for worker in 10.0.1.46: do
	ssh -n -f dvctrader@$worker '/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/scripts/auto_worker_restart.sh'
done
