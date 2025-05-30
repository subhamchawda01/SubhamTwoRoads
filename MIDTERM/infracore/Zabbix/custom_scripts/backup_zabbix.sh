#!/bin/bash 

#backup zabbix database 
mysqldump -u root zabbix > /spare1/ZabbixBackups/zabbix_database_backup_`date +"%Y%m%d"`.db 

for machine in `cat /root/list_of_all_machines.txt | grep -v "#"`
do
  scp $machine:/etc/zabbix/zabbix_agentd.conf /spare1/ZabbixBackups/zabbix_agentd_`date +"%Y%m%d"`"_"$machine".conf" ; 
done 

#remove older unnecessary databackups, older than 7 days 
find /spare1/ZabbixBackups/ -type f -mtime +7 -exec rm -f {} \;
