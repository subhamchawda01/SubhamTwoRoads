#!/bin/bash

USAGE="$0 EXCH PROFILE CMD";
if [ $# -ne 3 ] ; 
then 
    echo $USAGE;
#    exit;
fi

ORS_EXEC=$HOME/infracore_install/bin/cme_ilink_ors
#ORS_CONTROL_EXEC=$HOME/infracore_install/bin/ors_control_exec

EXCH=$1; shift;
PROFILE=$1; shift;
CMD=$1; shift;

echo "EXCH:" $EXCH "PROFILE:" $PROFILE "CMD:" $CMD;

emacs &
SERV_PID=$!
echo $SERV_PID

