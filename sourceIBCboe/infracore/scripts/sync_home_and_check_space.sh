#!/bin/bash 
#script runs on 5.67 checks all the local machines are reachable, checks home disk space and creates the backup all the 
declare -A server_ip_map

rm -rf /tmp/space_info.txt

server_ip_map=(["local67"]="10.23.5.67"
               ["local66"]="10.23.5.66"
               ["local50"]="10.23.5.50"
               ["local42"]="10.23.5.42"
	       ["local22"]="10.23.5.22"
	       ["local13"]="10.23.5.13"
               ["local62"]="10.23.5.62"
	       ["local26"]="10.23.5.26"
               ["local43"]="10.23.5.43")

for server in "${!server_ip_map[@]}";
do
  disk_home=`ssh ${server_ip_map[$server]} "df /home"`
  disk_root=`ssh ${server_ip_map[$server]} "df /"`
  space_home=`echo $disk_home | awk '{print $12}' | awk 'BEGIN{FS="%"}{print $1}'`
  space_root=`echo $disk_root | awk '{print $12}' | awk 'BEGIN{FS="%"}{print $1}'`
  if [ $space_home -ge 90 ] || [ $space_root -ge 90 ];then
    echo "/home or / is more than 90 % on : ${server}" >> /tmp/space_info.txt  
  fi
done

date=`date +"%Y%m%d"`

#get the backup of crontab of root, dvcinfra dvctrader
#5.67
crontab -u dvcinfra -l > /home/dvcinfra/important/CRON_BKP/dvcinfra_cron.bkp.${date}
crontab -u dvctrader -l > /home/dvcinfra/important/CRON_BKP/dvctrader_cron.bkp.${date}
crontab -l > /home/dvcinfra/important/CRON_BKP/root_cron.bkp.${date}
find /home/dvcinfra/important/CRON_BKP -type -f -name "*cron.bkp*" -mtime +6 -exec rm -f {} \;
#5.26
ssh 10.23.5.26 "find /home/dvcinfra/important/CRON_BKP -type -f -name "*cron.bkp*" -mtime +6 -exec rm -f {} \;"
ssh 10.23.5.26 "crontab -u dvcinfra -l > /home/dvctrader/important/CRON_BKP/dvcinfra_cron.bkp.${date}"
ssh 10.23.5.26 "crontab -u dvctrader -l > /home/dvctrader/important/CRON_BKP/dvctrader_cron.bkp.${date}"
ssh 10.23.5.26 "crontab -l > /home/dvctrader/important/CRON_BKP/root_cron.bkp.${date}"

#backup entire dvcinfra dvctrader and pengine folders
rsync -avz --progress --exclude=".*"  /home/dvcinfra /home/dvctrader /home/pengine 10.23.5.50:/home
if [ $? -ne 0 ];then
  echo "FAILED SYNCING HOME : 5.67" >> /tmp/space_info.txt
fi

ssh  10.23.5.26 "rsync  -avz --progress --exclude=".*" --exclude="ORSBCAST_MULTISHM"  --exclude="trash" /home/spare /home/dvcinfra /home/dvctrader /home/pengine 10.23.5.43:/home"

if [ $? -ne 0 ];then
  echo "FAILED SYNCING HOME : 5.26" >> /tmp/space_info.txt
fi

if [ -f /tmp/space_info.txt ];then
   cat /tmp/space_info.txt | mailx -s "SPACE ALERT AND LOCAL /home BACKUP" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi


