#!/bin/bash

function syncSingleFile {
	for i in "$@";  do
        echo $i
        dir=`dirname $i`
        ssh -o ConnectTimeout=60 54.208.92.178 "mkdir -p /mnt/sdf/$dir"
        rsync -ravz --timeout=60 $i 54.208.92.178:/mnt/sdf/$dir
        ssh -o ConnectTimeout=60 54.208.92.178 "sh /mnt/sdf/basetrade/AwsScripts/update_worker_execs.sh $i"
    done;
}

if [ $# -gt 0 ]; then
		extension=`echo $1 | awk -F'.' '{print $NF}'`
		if [ "$extension" == "list" ]; then
			lines=`cat $1`
			for line in $lines; do
				syncSingleFile $line
			done
			exit $?;
		else
			syncSingleFile	"$@"
	    exit $?;
    fi
fi

#Pulling latest code of basetrade
SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
ssh -n -f  $SSH_VARS 54.208.92.178 "cd basetrade; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS 54.208.92.178 "cd infracore; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS 54.208.92.178 "cd dvctrade; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS 54.208.92.178 "cd dvccode; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS 54.208.92.178 "cd baseinfra; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS 54.208.92.178 "cd dvc-bardata-toolkit; git reset --hard; git pull &>/dev/null &"

rsync -ravz --timeout=60 --exclude=bindebug --exclude="*~" --delete-after /spare1/shared/cvquant_install/.  54.208.92.178:/mnt/sdf/cvquant_install/
rsync -ravz --timeout=60 --exclude=bindebug --exclude="*~" --delete-after /home/dvctrader/LiveExec/.  54.208.92.178:/mnt/sdf/LiveExec/
rsync -ravz --timeout=60 /spare/local/files/. 54.208.92.178:/mnt/sdf/spare_local_files/
rsync -ravz --timeout=60 /spare/local/L1Norms/. 54.208.92.178:/mnt/sdf/spare_local_L1Norms/
rsync -ravz --timeout=60 /NAS1/data/MFGlobalTrades/EODPnl/. 54.208.92.178:/apps/data/MFGlobalTrades/EODPnl/

# rsync -ravz --timeout=60 --delete-after /NAS1/data/ORSData/CONFUPDATE  54.208.92.178:/mnt/sdf/
# rsync -ravz --timeout=60 --delete-after /NAS1/data/ORSData/CXLCONF  54.208.92.178:/mnt/sdf/
# rsync -ravz --timeout=60 --delete-after /NAS1/data/ORSData/SEQCONF  54.208.92.178:/mnt/sdf/

ssh -o ConnectTimeout=60 54.208.92.178 'sh /mnt/sdf/basetrade/AwsScripts/update_worker_execs.sh'

#Sync Startup Scripts
# rsync -avz --timeout=60 --delete-after /home/dvctrader/basetrade/AwsScripts/startup_settings_ec2-user.sh  54.208.92.178:/mnt/sdf/startup_settings_ec2-user.sh
# rsync -avz --timeout=60 --delete-after /home/dvctrader/basetrade/AwsScripts/startup_settings.sh  54.208.92.178:/mnt/sdf/startup_settings.sh
# rsync -avz --timeout=60 --delete-after /home/dvctrader/basetrade/AwsScripts/startup_settings_ec2-user_autoscaling.sh  54.208.92.178:/mnt/sdf/startup_settings_ec2-user_autoscaling.sh
# rsync -avz --timeout=60 --delete-after /home/dvctrader/basetrade/AwsScripts/startup_settings_autoscaling.sh  54.208.92.178:/mnt/sdf/startup_settings_autoscaling.sh

#Sync AWS perl scripts
rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" /home/dvctrader/basetrade/AwsScripts/run_cmd.pl 54.208.92.178:/home/dvctrader/controller_scripts/run_cmd.pl
