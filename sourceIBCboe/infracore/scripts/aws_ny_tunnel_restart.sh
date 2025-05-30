#!/bin/bash

USAGE="$0 user@ip message";


aws_server="dvctrader@10.0.1.46"
ny_server="dvctrader@10.23.74.51"

ssh $aws_server "/home/pengine/prod/live_scripts/scripts/aws_ny_connectivity_checker.sh $ny_server"

if [ $? != 0 ] ; then 
   echo "restarting the tunnel"
   tunnel_id=`ps -ef|grep maintain_tunnel|grep -v grep|awk '{print $2}'`
   kill  $tunnel_id > /dev/null
   tunnel_count=`ps -efH | grep maintain_tunnel | grep -v grep | awk '{print $2}' | wc -l`;
   if [ "$tunnel_count" -le 0 ]; then 
     sh -c "/home/dvctrader/LiveExec/scripts/maintain_tunnel.sh > /home/dvctrader/tunnel_log 2>&1 &"; 
   fi
fi
exit 0
