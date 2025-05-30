#!/bin/bash

ALL_SERVERS_CONFIG="/spare/local/tradeinfo/all_servers.cfg"
OUTPUT_FILE="/spare/local/logs/profilelogs/t2t_monthly_average_"`date +\%Y\%m\%d`
T2T_STATS_SCRIPT="/home/pengine/prod/live_scripts/t2t_analysis.sh"

for server in `cat $ALL_SERVERS_CONFIG | grep -v "#"`
do
  echo "Trying "$server
  is_accessible=`ssh $server "echo true"`;
  if [ -z $is_accessible ] || [ $is_accessible != "true" ] 
  then
    echo "Server $server not accessible"
    continue
  fi

  ssh -q "dvcinfra@"$server /bin/bash << EOF
  if [ -d /spare/local/logs/profilelogs/ ]
  then
     "$T2T_STATS_SCRIPT" 30 > $OUTPUT_FILE 2>&1 & 
  fi
EOF
 
done
