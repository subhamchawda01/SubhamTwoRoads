#!/bin/bash

USAGE="$0 TRD_MAC EXEC";
if [ $# -ne 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRD_USER=dvcinfra
TRD_MAC=$1; shift;
EXEC=$1; shift;

ssh $TRD_USER@$TRD_MAC "mkdir -p ~/infracore_install/bin"
rsync --quiet ~/infracore_install/bin/$EXEC $TRD_USER@$TRD_MAC:/home/$TRD_USER/infracore_install/bin/
#rsync --quiet ~/infracore_install/bindebug/$EXEC $TRD_USER@$TRD_MAC:/home/$TRD_USER/infracore_install/bindebug/
ssh $TRD_USER@$TRD_MAC "~/infracore/scripts/set_live_exec.sh $EXEC"
