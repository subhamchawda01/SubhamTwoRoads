#!/bin/bash

host_name=`hostname`
ors_control_exec="/home/pengine/prod/live_execs/ors_control_exec"

if [ "${host_name:4:3}" != "ind" ]; then
	ors_config_dir="/home/pengine/prod/live_configs"
	ors_profiles=`crontab -l |  grep -i "SmartDaemon.*ORS .*START" | cut -d ' ' -f8 | sort -u `
else
	ors_config_dir="$HOME/infracore_install/Configs/OrderRoutingServer/cfg"
	ors_profiles=`crontab -l | grep -i ors_control.*START | cut -d ' ' -f8 | sort -u`
fi

for profile in $ors_profiles;
do
	if [ "${host_name:4:3}" != "ind" ]; then
		ors_config_file="$ors_config_dir/common_${profile}_ors.cfg"
	else
		ors_config_file="$ors_config_dir/$profile/ors.cfg"
	fi

	if [ -e $ors_config_file ]; then
		control_port=`cat $ors_config_file | grep Control_Port | awk '{print $2}'`
		if [  "$control_port" != "" ]; then
			$ors_control_exec $control_port RELOADMARGINFILE
		fi
	fi
done
