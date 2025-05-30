#!/bin/bash

USAGE="$0 TRD_MAC SCRIPT_PATH_FROM_BASETRADE";
if [ $# -ne 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRD_USER=dvctrader
TRD_MAC=$1; shift;
SCRIPT_PATH=$1; shift;

if [ -e ~/basetrade/$SCRIPT_PATH ] ; then
    ssh $TRD_USER@$TRD_MAC "mkdir -p ~/LiveExec"
    rsync --quiet ~/basetrade/$SCRIPT_PATH $TRD_USER@$TRD_MAC:/home/$TRD_USER/LiveExec/$SCRIPT_PATH
fi