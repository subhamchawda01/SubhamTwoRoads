#!/bin/bash

erverList=("10.23.5.66" "10.23.5.67" "10.23.5.13" "10.23.5.42" "10.23.5.43" "10.23.5.26" "10.23.5.22" "10.23.5.68" "10.23.5.62" "10.23.5.69" "10.23.5.68")
true>/tmp/deadLocalServerList
>/tmp/dead_serer_lost
for server in "${serverList[@]}"
do
	 
#	if ! (ping -c 1 -W 1 $server)  ; then
	if ssh -o ConnectTimeout=9 "dvcinfra@$server" false >>/tmp/dead_serer_lost 2>&1; then
    		echo $server >> /tmp/deadLocalServerList
	fi
done
if [ `cat /tmp/deadLocalServerList | wc -l` -gt 0 ]; then
cat /tmp/deadLocalServerList | mailx -s "Currently down LOCAL Server List" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in #ravi.parikh@tworoads.co.in
fi

if [ `cat /tmp/dead_serer_lost | grep -v "banner exchange" | wc -l` -gt 0 ]; then
echo "" | mailx -s "Currently down LOCAL `cat /tmp/dead_serer_lost`" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in #ravi.parikh@tworoads.co.in
fi

