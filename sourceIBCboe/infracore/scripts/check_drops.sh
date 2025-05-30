#!/bin/bash

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : p4p2" ;
  exit;
fi

echo "DROPS AT ADAPTER LEVEL"
sudo ethtool -S $1 | grep drop

echo "DROPS AT SOCKET LEVEL"
onload_stackdump lots | grep oflow_drop

echo "memory pressure"
onload_stackdump lots | grep memory_pressure


echo "ONLOAD WATCH COMMAND"

echo  "watch -n1 'onload_stackdump lots | grep \"EF_MAX_PACKETS\|EF_MAX_RX\|EF_MAX_TX\|udp_recv\|u_poll\|memory_pressure\|EF_UDP_RCVBUF\|udp_tot_recv_drops\|pkt_bufs\"'"


echo "LINK: https://ref.onixs.biz/lost-multicast-packets-troubleshooting.html"
echo ""Onload User Guide", the "Eliminating Drops" and "Identifying Memory Pressure" sections."
