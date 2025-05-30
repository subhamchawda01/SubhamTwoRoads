#!/bin/bash
#Checks all workers for presence of file provided as argument (/NAS1/data/..)
file_path=$1
response="0"
for worker in 10.0.1.57 10.0.1.139 10.0.1.247 10.0.1.248
  do
    response=`ssh  -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o ConnectTimeout=5 $worker 'bash -s' < check_presence.sh $file_path`
    if [[ ${response:0:6} == "/media" ]]; then
      echo $worker":"$response
      break
    fi
done

if [[ ${response:0:1} == "0" ]]; then
  echo "0"
fi
