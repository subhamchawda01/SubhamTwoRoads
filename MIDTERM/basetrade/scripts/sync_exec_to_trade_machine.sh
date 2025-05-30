#!/bin/bash

USAGE="$0 TRD_MAC EXEC";
if [ $# -ne 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRD_USER=dvctrader
TRD_MAC=$1; shift;
EXEC=$1; shift;

ssh $TRD_USER@$TRD_MAC "mkdir -p ~/basetrade_install/bin"
rsync --quiet ~/basetrade_install/bin/$EXEC $TRD_USER@$TRD_MAC:/home/$TRD_USER/basetrade_install/bin/
#rsync --quiet ~/basetrade_install/bindebug/$EXEC $TRD_USER@$TRD_MAC:/home/$TRD_USER/basetrade_install/bindebug/
ssh $TRD_USER@$TRD_MAC "~/basetrade/scripts/set_live_exec.sh $EXEC"
