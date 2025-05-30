#!/bin/bash
HOSTN=`hostname -s`;
if [ -e ~/gsq/$HOSTN ] ; 
then
    mv ~/gsq/$HOSTN ~/gsq/_$HOSTN
fi
