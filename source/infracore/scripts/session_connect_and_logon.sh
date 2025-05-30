#!/bin/bash

if [ $# -lt 4 ]
then
  echo "USAGE: SCRIPT EXCH SESSION BCASTEXCH [VMA/ONLOAD/ON/OFF] [CLEAR/KEEP] [LOGIN time in seconds]"
  exit
fi

EXCH=$1
SESSION=$2
BCASTEXCH=$3
VMA=$4
CLEAR=$5
LOGINTIME=15

if [ ! -z $6 ] && [ $6 -gt 0 ]
then
  LOGINTIME=$6
fi

if [ "$VMA" == "VMA" ] || [ "$VMA" == "ONLOAD" ] || [ "$VMA" == "ON" ]
then
  /home/dvcinfra/LiveExec/OrderRoutingServer/onload_orsD.sh $EXCH $SESSION START $VMA $CLEAR
  sleep 15
fi
 
/home/dvcinfra/LiveExec/OrderRoutingServer/ors_control.pl $EXCH $SESSION START 
sleep $LOGINTIME
/home/dvcinfra/LiveExec/OrderRoutingServer/ors_control.pl $EXCH $SESSION LOGIN
sleep 30
/home/dvcinfra/LiveExec/scripts/fetch_ors_data_and_bcast.sh $VMA START $BCASTEXCH
