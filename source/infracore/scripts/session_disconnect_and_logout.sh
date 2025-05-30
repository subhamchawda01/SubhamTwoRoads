#!/bin/bash

if [ $# -lt 4 ]
then
  echo "USAGE: SCRIPT EXCH SESSION BCASTEXCH [VMA/ONLOAD/ON/OFF] [CLEAR/KEEP] [LOGOUT time in seconds]"
  exit
fi

EXCH=$1
SESSION=$2
BCASTEXCH=$3
VMA=$4
CLEAR=$5
LOGOUTTIME=30

if [ ! -z $6 ] && [ $6 -gt 0 ]
then
  LOGOUTTIME=$6
fi

/home/dvcinfra/LiveExec/OrderRoutingServer/ors_control.pl $EXCH $SESSION LOGOUT
sleep $LOGOUTTIME
/home/dvcinfra/LiveExec/OrderRoutingServer/ors_control.pl $EXCH $SESSION STOP

if [ "$VMA" == "VMA" ] || [ "$VMA" == "ONLOAD" ] || [ "$VMA" == "ON" ]
then
  sleep 15
  /home/dvcinfra/LiveExec/OrderRoutingServer/onload_orsD.sh $EXCH $SESSION STOP $VMA $CLEAR
fi

sleep 30
/home/dvcinfra/LiveExec/scripts/fetch_ors_data_and_bcast.sh $VMA STOP $BCASTEXCH
