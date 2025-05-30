#!/bin/bash

function syncSingleFile {
	for i in "$@";  do
        echo $i
        dir=`dirname $i`
        ssh -o ConnectTimeout=60 -p 7374 $SERV_IP "mkdir -p $dir"
        rsync -ravz --timeout=60 -e "ssh -p 7374" $i $SERV_IP:$i
    done;
}

if [ $# -lt 1 ]; then
    echo "$0 worker-ip [files/dirs to sync]"
    exit;
fi

SERV_IP=$1; shift;
    
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
SSH_VARS="-p 7374 -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
ssh -n -f  $SSH_VARS $SERV_IP "cd basetrade; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS $SERV_IP "cd infracore; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS $SERV_IP "cd dvctrade; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS $SERV_IP "cd dvccode; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS $SERV_IP "cd baseinfra; git reset --hard; git pull &>/dev/null &"
ssh -n -f  $SSH_VARS $SERV_IP "cd dvc-bardata-toolkit; git reset --hard; git pull &>/dev/null &"

rsync -ravz --timeout=60 -e "ssh -p 7374" --exclude=bindebug --exclude="*~" --exclude=*.a --delete-after /spare1/shared/cvquant_install/.  $SERV_IP:/home/dvctrader/cvquant_install/
rsync -ravz --timeout=60 -e "ssh -p 7374" --exclude=bindebug --exclude="*~" --delete-after /home/dvctrader/LiveExec/.  $SERV_IP:/home/dvctrader/LiveExec/
rsync -ravz --timeout=60 -e "ssh -p 7374" /spare/local/files/. $SERV_IP:/mnt/sdf/spare_local_files/
rsync -ravz --timeout=60 -e "ssh -p 7374" /spare/local/L1Norms/. $SERV_IP:/mnt/sdf/spare_local_L1Norms/
rsync -ravz --timeout=60 -e "ssh -p 7374" /NAS1/data/MFGlobalTrades/EODPnl/. $SERV_IP:/apps/data/MFGlobalTrades/EODPnl/

# Copy execs from dvctrade/dvccode to basetrade_install
ssh -n -f  $SSH_VARS $SERV_IP "rsync -ravz --timeout=60 --exclude=bindebug  --exclude=*.a /home/dvctrader/cvquant_install/dvctrade/bin /home/dvctrader/cvquant_install/basetrade/";
ssh -n -f  $SSH_VARS $SERV_IP "rsync -ravz --timeout=60 --exclude=bindebug  --exclude=*.a /home/dvctrader/cvquant_install/dvccode/bin /home/dvctrader/cvquant_install/basetrade/";
ssh -n -f  $SSH_VARS $SERV_IP "rsync -ravz --timeout=60 --exclude=bindebug  --exclude=*.a /home/dvctrader/cvquant_install/dvccode/bin /home/dvctrader/cvquant_install/infracore/";

# Sync the eco files
syncSingleFile "/home/pengine/infracore/SysInfo/BloombergEcoReports/merged_eco_$(date +%Y)_processed.txt";
syncSingleFile "/home/pengine/infracore/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt";
syncSingleFile "/spare/local/logs/holiday_manager_logs";

# Sync CeleryFiles
# ssh -n -f  $SSH_VARS $SERV_IP "rsync -ravz --timeout=60 /home/dvctrader/dvccode/scripts/datainfra/celeryFiles ec2-user@0:";

#Ensure /apps/data/MFGlobalTrades is updated

# Sync Pengine Execs (currently done in ny11)
#rsync -avz --timeout=60 --delete-after -e "ssh -p 7374" /home/pengine/prod/. $SERV_IP:/mnt/sdf/pengine/prod

#Sync AWS perl scripts
#rsync -ravz --timeout=60 -e "ssh -p 7374 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" /home/dvctrader/basetrade/AwsScripts/run_cmd.pl $SERV_IP:/home/dvctrader/controller_scripts/run_cmd.pl
