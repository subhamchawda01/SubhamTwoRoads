#!/bin/bash
declare -a server_ips
server_ips=("10.23.227.61" "10.23.227.62"  "10.23.227.64"  "10.23.227.65" "10.23.227.81" "10.23.227.82" "10.23.227.83" "10.23.227.84" "10.23.227.69")
user="dvcinfra"
for ip in ${server_ips[@]};
do
	ssh $user@$ip "
	rm ~/trash/*reject*
	rm ~/trash/core*
	rm ~/core*
	rm /tmp/*reject*
	"
done;
