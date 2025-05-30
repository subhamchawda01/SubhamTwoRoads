#!/bin/bash

USAGE="$0 EXECNAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

EXECNAME=$1
shift;

mkdir -p $HOME/LiveExec/bin ;
mkdir -p $HOME/LiveExec/bindebug ;

# install -p --backup=t -t $HOME/LiveExec/bindebug/ $HOME/infracore_install/bindebug/$EXECNAME
if [ -d $HOME/infracore_install/bin ] ; then
    install -p --backup=t -t $HOME/LiveExec/bin/ $HOME/infracore_install/bin/$EXECNAME
else
    HOSTNAME=`hostname -s`;
    echo "On $HOSTNAME missing $HOME/infracore_install/bin" ;
fi

#find $HOME/LiveExec -name *~ -type f -mtime +15 -exec rm -f {} \;
