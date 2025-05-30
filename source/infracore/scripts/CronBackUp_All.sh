#!/bin/bash 
Cron_Dir='/home/dvcinfra/cron_bkp/'
mkdir -p $Cron_Dir

declare -A server_ip_map
declare -A IND_Server_ip

server_ip_map=(["local13"]="10.23.5.13"
               ["local67"]="10.23.5.67"
               ["local66"]="10.23.5.66"
               ["local42"]="10.23.5.42"
               ["local43"]="10.23.5.43")

IND_Server_ip=( ["IND11"]="10.23.227.61" \
                ["IND12"]="10.23.227.62" \
                ["IND13"]="10.23.227.63" \
                ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
                ["IND20"]="10.23.227.84")



date=`date +"%Y%m%d"`



find $Cron_Dir -type f -name "*cron.bkp*" -mtime +6 -exec rm -f {} \;


for server in "${!server_ip_map[@]}";
do
  ssh dvcinfra@${server_ip_map[$server]} "crontab -l" > "$Cron_Dir/${server}.dvcinfra_cron.bkp.${date}"
  ssh dvctrader@${server_ip_map[$server]} "crontab -l" > "$Cron_Dir/${server}.dvctrader_cron.bkp.${date}"
  ssh root@${server_ip_map[$server]} "crontab -l" > "$Cron_Dir/${server}.root_cron.bkp.${date}"
done

ssh dvcinfra@10.23.5.50 "crontab -l" > "$Cron_Dir/local50.dvcinfra_cron.bkp.${date}"
ssh root@10.23.5.50 "crontab -l" > "$Cron_Dir/local50.root_cron.bkp.${date}"

for server in "${!IND_Server_ip[@]}";
do
  ssh dvcinfra@${IND_Server_ip[$server]} "crontab -l" > "$Cron_Dir/${server}dvcinfra_cron.bkp.${date}"
  ssh dvctrader@${IND_Server_ip[$server]} "crontab -l" > "$Cron_Dir/${server}dvctrader_cron.bkp.${date}"
done

