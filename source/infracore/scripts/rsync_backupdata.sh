rsync -avz --bwlimit 6000 --quiet dvcinfra@10.23.74.51:/NAS1/data /apps
rsync -avz --bwlimit 6000 --quiet dvcinfra@10.23.74.51:/apps/logs /apps
rsync -avz --bwlimit 6000 --quiet dvcinfra@10.23.74.51:/backupdata/EUREXLoggedData /backupdata
