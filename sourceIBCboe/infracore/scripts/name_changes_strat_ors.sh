#!/bin/bash

#["IND18"]="10.23.227.83" \
declare -A server_to_ip_map
declare -A server_to_folder_map
declare -A server_to_bkpfolder_map
declare -A server_to_host_map

server_to_ip_map=( ["IND17"]="10.23.227.82" \
		   ["IND18"]="10.23.227.83" \
                   ["IND23"]="10.23.227.72" )

server_to_host_map=( ["IND17"]="sdv-ind-srv17" \
		   ["IND18"]="sdv-ind-srv18" \
                   ["IND23"]="sdv-ind-srv23" )


server_to_folder_map=( ["IND17"]="/home/dvctrader/ATHENA/CONFIG_CM_NON_FO_202109/" \
		   ["IND18"]="/home/dvctrader/ATHENA/CONFIG_NON_FO_MIDTERM/" \
                   ["IND23"]="/home/dvctrader/ATHENA/CONFIG_CM_NON_FO_202106/" )


server_to_bkpfolder_map=( ["IND17"]="/home/dvctrader/ATHENA/CONFIG_CM_NON_FO_202109_BKP_2010913/" \
		   ["IND18"]="/home/dvctrader/ATHENA/CONFIG_NON_FO_MIDTERM/" \
                   ["IND23"]="/home/dvctrader/ATHENA/CONFIG_CM_NON_FO_202106_BKP_20210602/" )



#Main 
if [ $# -ne 2 ] ; then
  echo "Called As : OLDSYMBOL[JUMPNET] NEWSYMBOL[WINPRO]" ;
  exit
fi

old_sym=$1
new_sym=$2
date_=`date +%Y%m%d`
addts_file_ind17="/home/pengine/prod/live_configs/${HOSTNAME}_addts.cfg"

for server in "${!server_to_ip_map[@]}";
do
	hostname=${server_to_host_map[$server]}
	echo "SERVER: $server IP: ${server_to_ip_map[$server]}  HOSTNAME: $hostname"
	echo "ADDTS FILE "
	ssh dvcinfra@${server_to_ip_map[$server]} "egrep \"NSE_$new_sym|NSE_$old_sym\" /home/pengine/prod/live_configs/${hostname}_addts.cfg"
	ssh pengine@${server_to_ip_map[$server]} "sed -i \"s/NSE_$old_sym/NSE_$new_sym/g\" /home/pengine/prod/live_configs/${hostname}_addts.cfg"
	ssh dvcinfra@${server_to_ip_map[$server]} "egrep \"NSE_$new_sym|NSE_$old_sym\" /home/pengine/prod/live_configs/${hostname}_addts.cfg"
	ssh dvcinfra@${server_to_ip_map[$server]} "cp /home/pengine/prod/live_configs/${hostname}_addts.cfg /home/dvcinfra/important/${hostname}_addts.cfg_${date_}"
continue	
	echo "Prod ATHENA Folder ${server_to_folder_map[$server]}"
	ssh dvctrader@${server_to_ip_map[$server]} "mv -vn ${server_to_folder_map[$server]}/NSE_${old_sym}_MM ${server_to_folder_map[$server]}/NSE_${new_sym}_MM"
	ssh dvctrader@${server_to_ip_map[$server]} "mv -vn ${server_to_folder_map[$server]}/NSE_${old_sym}_SQUAREOFF ${server_to_folder_map[$server]}/NSE_${new_sym}_SQUAREOFF"
	ssh dvctrader@${server_to_ip_map[$server]} "cd ${server_to_folder_map[$server]}/NSE_${new_sym}_MM/; sed -i \"s/$old_sym/$new_sym/g\" *"
	ssh dvctrader@${server_to_ip_map[$server]} "cd ${server_to_folder_map[$server]}/NSE_${new_sym}_SQUAREOFF/; sed -i \"s/$old_sym/$new_sym/g\" *"	
	echo "Prod ATHENA Folder ${server_to_bkpfolder_map[$server]}"
	ssh dvctrader@${server_to_ip_map[$server]} "mv -vn ${server_to_bkpfolder_map[$server]}/NSE_${old_sym}_MM ${server_to_bkpfolder_map[$server]}/NSE_${new_sym}_MM"
        ssh dvctrader@${server_to_ip_map[$server]} "mv -vn ${server_to_bkpfolder_map[$server]}/NSE_${old_sym}_SQUAREOFF ${server_to_bkpfolder_map[$server]}/NSE_${new_sym}_SQUAREOFF"
        ssh dvctrader@${server_to_ip_map[$server]} "sed -i \"s/$old_sym/$new_sym/g\" ${server_to_bkpfolder_map[$server]}/NSE_${new_sym}_MM/*"      
        ssh dvctrader@${server_to_ip_map[$server]} "sed -i \"s/$old_sym/$new_sym/g\" ${server_to_bkpfolder_map[$server]}/NSE_${new_sym}_SQUAREOFF/*"
	
	ssh dvctrader@${server_to_ip_map[$server]} "sed -i \"s/NSE_$old_sym/NSE_$new_sym/g\" ${server_to_folder_map[$server]}/PositionLimits.csv"
	echo "***Update Live File MANUALLLY INCASE OF NAME CHANGE TO DIFF FILE"
	echo "***ADD ENTRY TO CHANNEL.infot file in 5.26 trader"
	#ssh dvctrader@${server_to_ip_map[$server]} "sed -i \"s/NSE_$old_sym/NSE_$new_sym/g\" ${server_to_folder_map[$server]}/LIVE_FILE_*.csv"
	
done



