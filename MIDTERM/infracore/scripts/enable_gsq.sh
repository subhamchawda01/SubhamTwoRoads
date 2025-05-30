#!/bin/bash
HOSTN=`hostname -s`;
if [ -e ~/gsq/_$HOSTN ] ; 
then
    mv ~/gsq/_$HOSTN ~/gsq/$HOSTN
fi
