#!/bin/bash
date=`date +"%Y%m%d"`
if [ $# -eq 1 ];then
  date=$1
fi

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ "$is_holiday" == "1" ];then
  exit;
fi

hhmmss=`date +"%H%M%S"`

if [ $hhmmss -le 103000 ];then
  echo "cannot run during market hours";
  exit
fi

backup_report='/tmp/backup_report.txt'
rm -rf $backup_report
declare -A server_ip_map
server_ip_map=( ["INDB11"]="192.168.132.11" \
                ["INDB12"]="192.168.132.12")

##mountpoint=`df -a | grep /dev/sdd1`;
##usage=`echo $mountpoint | awk '{print $5}' | awk 'BEGIN{FS="%"}{print $1}'`;

##[ "$mountpoint" == "" ] && echo "Disk is not mounted :  /dev/sdd1" > $backup_report;
##[ $usage -ge 95 ] && echo "Disk usage is more than 95%, Current Usage: $usage => /dev/sdd1" >> $backup_report;
#commenting below code for few days 

if [ -f $backup_report ];then
    cat $backup_report | mailx -s "SERVER DATA & EXEC BACKUP " -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
    exit
fi

#sync ORSlogs
for server in "${!server_ip_map[@]}";
do
        ssh ${server_ip_map[$server]} "find /spare/local/ORSlogs -type f | grep -v "trade" | grep -v "gz" | xargs gzip -f"
	[ $? -ne 0 ] && { echo "$server => ORSlogs1 BACKUP FAILED" >> $backup_report; continue; }
	echo "rsync -avz --progress ${server_ip_map[$server]}:/spare/local/ORSlogs /run/media/root/Elements/SERVERDATA/$server"
	rsync -avz --progress ${server_ip_map[$server]}:/spare/local/ORSlogs /run/media/root/Elements/SERVERDATA/$server
	[ $? -ne 0 ] && echo "$server => ORSlogs2 BACKUP FAILED" >> $backup_report;
done
#sync tradelogs
for server in "${!server_ip_map[@]}";
do 
	ssh ${server_ip_map[$server]} "find /spare/local/logs/tradelogs/ -type f | grep -v "trades" | grep -v "gz" | xargs gzip -f"
	[ $? -ne 0 ] && { echo "$server => TRADElogs BACKUP FAILED" >> $backup_report; continue; }
	rsync -avz --progress ${server_ip_map[$server]}:/spare/local/logs/tradelogs /run/media/root/Elements/SERVERDATA/$server 
	[ $? -ne 0 ] && echo "$server => TRADElogs BACKUP FAILED" >> $backup_report;
done


#sync live_execs
for server in "${!server_ip_map[@]}";
do 
	rsync -avz --progress ${server_ip_map[$server]}:/home/pengine/prod /run/media/root/Elements/SERVERDATA/$server;
	[ $? -ne 0 ] && echo "$server => Live Execs FAILED" >> $backup_report; 
done
#sync trade execs
for server in "${!server_ip_map[@]}";
do
        if [ "$server" == "IND11" ] || [ "$server" == "IND12" ] || [ "$server" == "IND13" ];then
		continue;
	fi
	rsync -avz --progress ${server_ip_map[$server]}:/home/dvctrader/ATHENA /run/media/root/Elements/SERVERDATA/$server; 
	[ $? -ne 0 ] && echo "$server => Trade execs BACKUP FAILED" >> $backup_report;
done

#only remove if sync is successfull
if [ ! -f $backup_report ];then

	for server in "${!server_ip_map[@]}"
	do
		ssh ${server_ip_map[$server]} "find /spare/local/ORSlogs -mtime +30 -type f   -exec rm -f {} \;";
		ssh ${server_ip_map[$server]} "find /spare/local/logs/tradelogs -mtime +30 -type f  -exec rm -f {} \;";
	done
fi

#sync config set to dated dir
for server in "${!server_ip_map[@]}";
do
        if [ "$server" == "INDB11" ];then
                continue;
        fi
	#delete 15 days old config
	find  /run/media/root/Elements/SERVERDATA/$server/CONFIG_SET -maxdepth 1 -type d -mtime +15 -exec rm -rf {} \;
        mkdir -p /run/media/root/Elements/SERVERDATA/$server/CONFIG_SET/$date
        for file in `ssh ${server_ip_map[$server]} "crontab -l -u dvctrader | grep -v '^#' | grep run | awk '{if(NF == 6) print}'" | awk '{print $NF}'`;
        do
	   for configset in `ssh ${server_ip_map[$server]} "cat ${file} | grep -v '#' | grep ATHENA | awk '{print $3}'" | awk '{print $2}' | awk -F "/" '{print "/"$2"/"$3"/"$4"/"$5}'`;
	   do
                   rsync -avz --progress ${server_ip_map[$server]}:${configset} /run/media/root/Elements/SERVERDATA/$server/CONFIG_SET/$date 
                   [ $? -ne 0 ] && echo "${server} : ${configset} => FAILED"
           done
        done    
done



if [ -f $backup_report ];then
	cat $backup_report | mailx -s "SERVER DATA & EXECs BACKUP " -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in  hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
fi
