#!/bin/bash

declare -A server_to_ip_map

server_to_ip_map=( ["IND14"]="10.23.227.64" \
                   ["IND15"]="10.23.227.65" \
                   ["IND16"]="10.23.227.81" \
                   ["IND17"]="10.23.227.82" \
                   ["IND18"]="10.23.227.83" \
                   ["IND19"]="10.23.227.69" \
                   ["IND20"]="10.23.227.84" )

today_=`date +"%Y%m%d"`

>/tmp/verify_addt_mail_file

for server in "${!server_to_ip_map[@]}";
do

done

