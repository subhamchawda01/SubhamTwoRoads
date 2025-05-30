#!/bin/bash

serverList=("10.23.227.61" "10.23.227.62" "10.23.227.63" "10.23.227.64" "10.23.227.65" "10.23.227.81" "10.23.227.82" "10.23.227.83"  "10.23.227.69" "10.23.227.84" "10.23.227.66" "10.23.227.71")
true>/tmp/deadIndServerList
for server in "${serverList[@]}"
do
	 
#	if ! (ping -c 1 -W 1 $server)  ; then
	if ssh -o ConnectTimeout=9 "dvcinfra@$server" false; then
    		echo $server >> /tmp/deadIndServerList
	fi
done

if [`cat /tmp/deadIndServerList | wc -l` -gt 0 ]; then
cat /tmp/deadIndServerList | mailx -s "Currently down IND Server List" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in #ravi.parikh@tworoads.co.in
fi
